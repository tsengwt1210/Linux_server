"""Integration checks on 127.0.0.1; multicast is deliberately not started."""
import hashlib
import os
from pathlib import Path
import shutil
import signal
import socket
import struct
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "build" / "bin"


def receive_all(sock):
    chunks = []
    while True:
        chunk = sock.recv(8192)
        if not chunk:
            return b"".join(chunks)
        chunks.append(chunk)


@unittest.skipUnless(os.name == "posix", "Requires Linux/POSIX")
class LoopbackTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="linux-server-tests-")
        cls.addClassCleanup(cls.temp.cleanup)
        cls.base = Path(cls.temp.name)
        cls.uploads = cls.base / "uploads"
        cls.www = cls.base / "www"
        cls.uploads.mkdir()
        cls.www.mkdir()
        shutil.copy2(ROOT / "examples/www/index.html", cls.www / "index.html")
        shutil.copy2(BIN / "cgi", cls.www / "cgi")
        cls.outside = cls.base / "outside.html"
        cls.outside.write_text("private test fixture", encoding="utf-8")
        (cls.www / "linked.html").symlink_to(cls.outside)
        cls.start_server("tcp_server", 12345, cls.uploads)
        cls.start_server("http_server", 9998, cls.www)

    @classmethod
    def start_server(cls, program, port, cwd):
        # Refuse to contact an unrelated server if the fixed exercise port is busy.
        with socket.socket() as probe:
            probe.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            probe.bind(("127.0.0.1", port))
        log = open(cls.base / (program + ".log"), "wb")
        cls.addClassCleanup(log.close)
        process = subprocess.Popen(
            [str(BIN / program)], cwd=cwd, stdout=log, stderr=log,
            start_new_session=True,
        )
        cls.addClassCleanup(cls.stop_server, process)
        deadline = time.monotonic() + 5
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f"{program} exited before it started")
            try:
                with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                    return
            except OSError:
                time.sleep(0.02)
        raise TimeoutError(f"{program} did not start")

    @staticmethod
    def stop_server(process):
        # Kill only the new process group created above, including its forked children.
        try:
            os.killpg(process.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass
        try:
            process.wait(timeout=3)
        except subprocess.TimeoutExpired:
            os.killpg(process.pid, signal.SIGKILL)
            process.wait(timeout=3)

    def upload_with_client(self, data):
        with tempfile.TemporaryDirectory(dir=self.base) as work:
            Path(work, "homework.txt").write_bytes(data)
            before = set(self.uploads.glob("Client_*_homework.txt"))
            result = subprocess.run([str(BIN / "tcp_client")], cwd=work,
                                    capture_output=True, timeout=5)
            self.assertEqual(result.returncode, 0, result.stderr.decode(errors="replace"))
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                created = set(self.uploads.glob("Client_*_homework.txt")) - before
                if len(created) == 1:
                    saved = created.pop().read_bytes()
                    if len(saved) == len(data):
                        self.assertEqual(hashlib.sha256(saved).digest(), hashlib.sha256(data).digest())
                        return
                time.sleep(0.02)
            self.fail("Uploaded file was not saved with the expected content")

    def test_tcp_binary_file(self):
        self.upload_with_client(bytes(range(256)) * 4096 + b"tail")

    def test_tcp_empty_file(self):
        self.upload_with_client(b"")

    def test_tcp_missing_input_fails(self):
        with tempfile.TemporaryDirectory(dir=self.base) as work:
            result = subprocess.run([str(BIN / "tcp_client")], cwd=work,
                                    capture_output=True, timeout=5)
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(b"homework.txt", result.stderr)

    def test_tcp_fragmented_filename(self):
        name = b"fragmented.txt".ljust(256, b"\0")
        with socket.create_connection(("127.0.0.1", 12345), timeout=3) as sock:
            for start in range(0, 256, 17):
                sock.sendall(name[start:start + 17])
            offset = b""
            while len(offset) < struct.calcsize("@q"):
                part = sock.recv(struct.calcsize("@q") - len(offset))
                self.assertTrue(part)
                offset += part
            self.assertEqual(struct.unpack("@q", offset)[0], 0)
            sock.sendall(b"fragmented header succeeded")
            sock.shutdown(socket.SHUT_WR)
            self.assertEqual(receive_all(sock), b"")
        matches = list(self.uploads.glob("Client_*_fragmented.txt"))
        self.assertEqual(len(matches), 1)
        self.assertEqual(matches[0].read_bytes(), b"fragmented header succeeded")

    def test_tcp_rejects_path(self):
        with socket.create_connection(("127.0.0.1", 12345), timeout=3) as sock:
            sock.sendall(b"../outside.txt".ljust(256, b"\0"))
            sock.shutdown(socket.SHUT_WR)
            self.assertEqual(receive_all(sock), b"")
        self.assertFalse((self.base / "outside.txt").exists())

    def http(self, target="/", method="GET", fragments=False):
        request = f"{method} {target} HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n".encode()
        with socket.create_connection(("127.0.0.1", 9998), timeout=3) as sock:
            if fragments:
                sock.sendall(request[:8])
                time.sleep(0.02)
                sock.sendall(request[8:])
            else:
                sock.sendall(request)
            response = receive_all(sock)
        header, body = response.split(b"\r\n\r\n", 1)
        return int(header.split(b" ", 2)[1]), header, body

    def test_http_static_file(self):
        code, header, body = self.http()
        self.assertEqual(code, 200)
        self.assertIn(b"Content-Type: text/html", header)
        self.assertEqual(body, (self.www / "index.html").read_bytes())
        self.assertIn(f"Content-Length: {len(body)}".encode(), header)

    def test_http_fragmented_request(self):
        self.assertEqual(self.http(fragments=True)[0], 200)

    def test_http_cgi(self):
        code, _, body = self.http("/cgi")
        self.assertEqual(code, 200)
        for expected in (b"CGI Dynamic Page", b"Server Time:", b"127.0.0.1", b"Your port:"):
            self.assertIn(expected, body)

    def test_http_errors(self):
        for target, status in (("/missing.html", 404), ("/unsupported.txt", 415),
                               ("/../outside.html", 403), ("/%2e%2e/outside.html", 403),
                               ("/linked.html", 404), ("/cgi?query=1", 403)):
            with self.subTest(target=target):
                code, _, body = self.http(target)
                self.assertEqual(code, status)
                self.assertNotIn(b"private test fixture", body)
        self.assertEqual(self.http(method="POST")[0], 405)

    def test_http_large_header_rejected(self):
        with socket.create_connection(("127.0.0.1", 9998), timeout=3) as sock:
            sock.sendall(b"GET / HTTP/1.1\r\nX: ".ljust(1023, b"a"))
            response = receive_all(sock)
        self.assertTrue(response.startswith(b"HTTP/1.1 431 "))


if __name__ == "__main__":
    unittest.main()
