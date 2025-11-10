#!/bin/bash

echo "=========================================="
echo "  License Management System"
echo "=========================================="
echo ""
echo "Starting Flask server..."
echo ""
echo "Web Admin Panel will be available at:"
echo "  http://localhost:5000/admin/dashboard"
echo ""
echo "API Endpoints available at:"
echo "  http://localhost:5000/api/validate"
echo "  http://localhost:5000/api/admin/licenses"
echo ""
echo "Press Ctrl+C to stop the server"
echo "=========================================="
echo ""

python3 license_server.py
