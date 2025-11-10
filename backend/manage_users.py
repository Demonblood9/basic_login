#!/usr/bin/env python3
"""
User Management CLI Tool
This script helps you manage users and access keys for the login system.
"""

import requests
import sys
import json

BASE_URL = "http://localhost:5000/api"

def create_user(username):
    """Create a new user and generate an access key"""
    response = requests.post(
        f"{BASE_URL}/admin/create_user",
        json={"username": username},
        headers={"Content-Type": "application/json"}
    )

    if response.status_code == 201:
        data = response.json()
        print(f"\n✓ User created successfully!")
        print(f"Username: {data['username']}")
        print(f"Access Key: {data['access_key']}")
        print(f"\n{data['note']}")
        print("\n" + "="*60)
    else:
        print(f"✗ Error: {response.json().get('message', 'Unknown error')}")

def list_users():
    """List all users"""
    response = requests.get(f"{BASE_URL}/admin/list_users")

    if response.status_code == 200:
        data = response.json()
        users = data['users']

        if not users:
            print("\nNo users found.")
            return

        print(f"\n{'ID':<5} {'Username':<20} {'Status':<10} {'Created':<20} {'Last Login':<20}")
        print("="*80)

        for user in users:
            status = "Active" if user['is_active'] else "Inactive"
            last_login = user['last_login'][:19] if user['last_login'] else "Never"
            created = user['created_at'][:19]
            print(f"{user['id']:<5} {user['username']:<20} {status:<10} {created:<20} {last_login:<20}")
    else:
        print(f"✗ Error: {response.json().get('message', 'Unknown error')}")

def deactivate_user(user_id):
    """Deactivate a user"""
    response = requests.post(f"{BASE_URL}/admin/deactivate_user/{user_id}")

    if response.status_code == 200:
        print(f"✓ {response.json()['message']}")
    else:
        print(f"✗ Error: {response.json().get('message', 'Unknown error')}")

def activate_user(user_id):
    """Activate a user"""
    response = requests.post(f"{BASE_URL}/admin/activate_user/{user_id}")

    if response.status_code == 200:
        print(f"✓ {response.json()['message']}")
    else:
        print(f"✗ Error: {response.json().get('message', 'Unknown error')}")

def test_key(key):
    """Test if a key is valid"""
    response = requests.post(
        f"{BASE_URL}/validate",
        json={"key": key},
        headers={"Content-Type": "application/json"}
    )

    if response.status_code == 200:
        data = response.json()
        print(f"\n✓ Key is valid!")
        print(f"Username: {data['username']}")
        print(f"Last Login: {data['last_login']}")
    else:
        print(f"✗ {response.json().get('message', 'Invalid key')}")

def print_usage():
    """Print usage information"""
    print("""
Usage: python manage_users.py <command> [arguments]

Commands:
    create <username>       Create a new user and generate an access key
    list                    List all users
    deactivate <user_id>    Deactivate a user by ID
    activate <user_id>      Activate a user by ID
    test <key>              Test if an access key is valid

Examples:
    python manage_users.py create john_doe
    python manage_users.py list
    python manage_users.py deactivate 1
    python manage_users.py test abc123def456...
""")

def main():
    if len(sys.argv) < 2:
        print_usage()
        sys.exit(1)

    command = sys.argv[1].lower()

    try:
        if command == "create":
            if len(sys.argv) < 3:
                print("✗ Error: Username required")
                print("Usage: python manage_users.py create <username>")
                sys.exit(1)
            create_user(sys.argv[2])

        elif command == "list":
            list_users()

        elif command == "deactivate":
            if len(sys.argv) < 3:
                print("✗ Error: User ID required")
                print("Usage: python manage_users.py deactivate <user_id>")
                sys.exit(1)
            deactivate_user(int(sys.argv[2]))

        elif command == "activate":
            if len(sys.argv) < 3:
                print("✗ Error: User ID required")
                print("Usage: python manage_users.py activate <user_id>")
                sys.exit(1)
            activate_user(int(sys.argv[2]))

        elif command == "test":
            if len(sys.argv) < 3:
                print("✗ Error: Key required")
                print("Usage: python manage_users.py test <key>")
                sys.exit(1)
            test_key(sys.argv[2])

        else:
            print(f"✗ Unknown command: {command}")
            print_usage()
            sys.exit(1)

    except requests.exceptions.ConnectionError:
        print("\n✗ Error: Cannot connect to the server.")
        print("Make sure the Flask backend is running on http://localhost:5000")
        sys.exit(1)
    except Exception as e:
        print(f"\n✗ Error: {str(e)}")
        sys.exit(1)

if __name__ == "__main__":
    main()
