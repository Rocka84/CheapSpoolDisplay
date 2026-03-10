#!/usr/bin/env python3
import http.server
import json
import socketserver
from datetime import datetime

PORT = 8080

class WebhookHandler(http.server.BaseHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers['Content-Length'])
        post_data = self.rfile.read(content_length)
        
        print(f"\n[{datetime.now().strftime('%H:%M:%S')}] Received POST request")
        print(f"Protocol: {self.protocol_version}")
        print("Headers:")
        for name, value in self.headers.items():
            print(f"  {name}: {value}")
        
        try:
            data = json.loads(post_data.decode('utf-8'))
            print("Payload (JSON):")
            print(json.dumps(data, indent=2))
        except:
            print("Payload (Raw):")
            print(post_data.decode('utf-8'))

        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps({"status": "success"}).encode('utf-8'))

    def do_GET(self):
        print(f"\n[{datetime.now().strftime('%H:%M:%S')}] Received GET request")
        print(f"URL: {self.path}")
        print(f"Protocol: {self.protocol_version}")
        print("Headers:")
        for name, value in self.headers.items():
            print(f"  {name}: {value}")

        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps({"status": "success"}).encode('utf-8'))

if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), WebhookHandler) as httpd:
        print(f"Webhook Test Server running on port {PORT}")
        print(f"Configure your device webhook to: http://<your-ip>:{PORT}/")
        print("Press Ctrl+C to stop")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nStopping server...")
