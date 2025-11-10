#!/usr/bin/env python3
"""
OAuth 2.0 Client Management Tool
"""

import requests
import sys
import json

BASE_URL = "http://localhost:5000"

def create_client(name):
    """Create a new OAuth 2.0 client"""
    response = requests.post(
        f"{BASE_URL}/admin/clients",
        json={"client_name": name},
        headers={"Content-Type": "application/json"}
    )

    if response.status_code == 201:
        data = response.json()
        print(f"\n✓ OAuth 2.0 Client created successfully!")
        print(f"\nClient Name: {data['client_name']}")
        print(f"Client ID: {data['client_id']}")
        print(f"Client Secret: {data['client_secret']}")
        print(f"Grant Types: {data['grant_types']}")
        print(f"Scopes: {data['scopes']}")
        print(f"\n{data['note']}")
        print("\n" + "="*70)
        print("\nTo get an access token, use:")
        print(f"curl -X POST {BASE_URL}/oauth/token \\")
        print(f"  -d 'grant_type=client_credentials' \\")
        print(f"  -d 'client_id={data['client_id']}' \\")
        print(f"  -d 'client_secret={data['client_secret']}'")
        print("="*70)
    else:
        print(f"✗ Error: {response.json().get('error', 'Unknown error')}")

def list_clients():
    """List all OAuth 2.0 clients"""
    response = requests.get(f"{BASE_URL}/admin/clients")

    if response.status_code == 200:
        data = response.json()
        clients = data['clients']

        if not clients:
            print("\nNo clients found.")
            return

        print(f"\n{'ID':<5} {'Client Name':<25} {'Client ID':<35} {'Status':<10} {'Grant Types':<30}")
        print("="*110)

        for client in clients:
            status = "Active" if client['is_active'] else "Inactive"
            print(f"{client['id']:<5} {client['client_name']:<25} {client['client_id']:<35} {status:<10} {client['grant_types']:<30}")
    else:
        print(f"✗ Error: {response.json().get('error', 'Unknown error')}")

def get_token(client_id, client_secret):
    """Get an access token using client credentials"""
    response = requests.post(
        f"{BASE_URL}/oauth/token",
        data={
            'grant_type': 'client_credentials',
            'client_id': client_id,
            'client_secret': client_secret
        }
    )

    if response.status_code == 200:
        data = response.json()
        print(f"\n✓ Access token obtained successfully!")
        print(f"\nAccess Token: {data['access_token']}")
        print(f"Token Type: {data['token_type']}")
        print(f"Expires In: {data['expires_in']} seconds ({data['expires_in']//60} minutes)")
        print(f"Refresh Token: {data['refresh_token']}")
        print(f"Scope: {data['scope']}")
        print("\n" + "="*70)
        print("\nTo access protected resources, use:")
        print(f"curl -H 'Authorization: Bearer {data['access_token']}' \\")
        print(f"  {BASE_URL}/api/protected")
        print("="*70)
    else:
        error_data = response.json()
        print(f"✗ Error: {error_data.get('error', 'Unknown error')}")
        print(f"Description: {error_data.get('error_description', 'No description')}")

def test_token(access_token):
    """Test an access token"""
    response = requests.get(
        f"{BASE_URL}/api/protected",
        headers={'Authorization': f'Bearer {access_token}'}
    )

    if response.status_code == 200:
        data = response.json()
        print(f"\n✓ Token is valid!")
        print(f"Client: {data['client_name']}")
        print(f"Scope: {data['scope']}")
        print(f"Expires At: {data['expires_at']}")
    else:
        error_data = response.json()
        print(f"✗ Token validation failed")
        print(f"Error: {error_data.get('error', 'Unknown error')}")
        print(f"Description: {error_data.get('error_description', 'No description')}")

def introspect_token(token):
    """Introspect a token"""
    response = requests.post(
        f"{BASE_URL}/oauth/introspect",
        data={'token': token}
    )

    if response.status_code == 200:
        data = response.json()
        if data['active']:
            print(f"\n✓ Token is active!")
            print(f"Client ID: {data['client_id']}")
            print(f"Scope: {data['scope']}")
            print(f"Issued At: {data['iat']}")
            print(f"Expires At: {data['exp']}")
        else:
            print("\n✗ Token is not active (expired or revoked)")
    else:
        print(f"✗ Error: {response.status_code}")

def revoke_token(token):
    """Revoke a token"""
    response = requests.post(
        f"{BASE_URL}/oauth/revoke",
        data={'token': token}
    )

    if response.status_code == 200:
        print("\n✓ Token revoked successfully")
    else:
        print(f"✗ Error: {response.status_code}")

def print_usage():
    """Print usage information"""
    print("""
OAuth 2.0 Client Management Tool
=================================

Usage: python oauth_manage.py <command> [arguments]

Commands:
    create <name>                   Create a new OAuth 2.0 client
    list                            List all clients
    token <client_id> <secret>      Get access token
    test <access_token>             Test an access token
    introspect <token>              Introspect a token
    revoke <token>                  Revoke a token

Examples:
    python oauth_manage.py create "My Desktop App"
    python oauth_manage.py list
    python oauth_manage.py token abc123 def456
    python oauth_manage.py test eyJhbGc...
    python oauth_manage.py introspect eyJhbGc...
    python oauth_manage.py revoke eyJhbGc...
""")

def main():
    if len(sys.argv) < 2:
        print_usage()
        sys.exit(1)

    command = sys.argv[1].lower()

    try:
        if command == "create":
            if len(sys.argv) < 3:
                print("✗ Error: Client name required")
                print("Usage: python oauth_manage.py create <name>")
                sys.exit(1)
            create_client(sys.argv[2])

        elif command == "list":
            list_clients()

        elif command == "token":
            if len(sys.argv) < 4:
                print("✗ Error: Client ID and secret required")
                print("Usage: python oauth_manage.py token <client_id> <client_secret>")
                sys.exit(1)
            get_token(sys.argv[2], sys.argv[3])

        elif command == "test":
            if len(sys.argv) < 3:
                print("✗ Error: Access token required")
                print("Usage: python oauth_manage.py test <access_token>")
                sys.exit(1)
            test_token(sys.argv[2])

        elif command == "introspect":
            if len(sys.argv) < 3:
                print("✗ Error: Token required")
                print("Usage: python oauth_manage.py introspect <token>")
                sys.exit(1)
            introspect_token(sys.argv[2])

        elif command == "revoke":
            if len(sys.argv) < 3:
                print("✗ Error: Token required")
                print("Usage: python oauth_manage.py revoke <token>")
                sys.exit(1)
            revoke_token(sys.argv[2])

        else:
            print(f"✗ Unknown command: {command}")
            print_usage()
            sys.exit(1)

    except requests.exceptions.ConnectionError:
        print("\n✗ Error: Cannot connect to the OAuth 2.0 server.")
        print("Make sure the server is running on http://localhost:5000")
        sys.exit(1)
    except Exception as e:
        print(f"\n✗ Error: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
