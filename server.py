"""Local development server for the Nintendo Streaminet concept site."""

from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

PORT = 8000
ROOT = Path(__file__).resolve().parent


class StreaminetHandler(SimpleHTTPRequestHandler):
    """Serve static Streaminet files with small helpful defaults."""

    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(ROOT), **kwargs)

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()


if __name__ == "__main__":
    address = ("", PORT)
    with ThreadingHTTPServer(address, StreaminetHandler) as server:
        print(f"Nintendo Streaminet is live at http://localhost:{PORT}")
        server.serve_forever()
