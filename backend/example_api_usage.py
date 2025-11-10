#!/usr/bin/env python3
"""
Example API Usage
Demonstrates how to interact with the Secure Login API
"""

import requests
import json

BASE_URL = "http://localhost:5000/api"

def example_create_user():
    """Example: Create a new user"""
    print("\n=== Creating a new user ===")

    response = requests.post(
        f"{BASE_URL}/admin/create_user",
        json={"username": "example_user"},
        headers={"Content-Type": "application/json"}
    )

    print(f"Status Code: {response.status_code}")
    print(f"Response: {json.dumps(response.json(), indent=2)}")

    return response.json().get('access_key')

def example_validate_key(key):
    """Example: Validate an access key"""
    print("\n=== Validating access key ===")

    response = requests.post(
        f"{BASE_URL}/validate",
        json={"key": key},
        headers={"Content-Type": "application/json"}
    )

    print(f"Status Code: {response.status_code}")
    print(f"Response: {json.dumps(response.json(), indent=2)}")

def example_list_users():
    """Example: List all users"""
    print("\n=== Listing all users ===")

    response = requests.get(f"{BASE_URL}/admin/list_users")

    print(f"Status Code: {response.status_code}")
    data = response.json()

    if data['success']:
        print(f"\nTotal users: {len(data['users'])}")
        for user in data['users']:
            print(f"  - {user['username']} (ID: {user['id']}, Active: {user['is_active']})")

def example_health_check():
    """Example: Check server health"""
    print("\n=== Health check ===")

    response = requests.get(f"{BASE_URL}/health")

    print(f"Status Code: {response.status_code}")
    print(f"Response: {json.dumps(response.json(), indent=2)}")

def main():
    """Run all examples"""
    print("Secure Login API - Example Usage")
    print("=" * 50)

    try:
        # Health check
        example_health_check()

        # Create a user
        access_key = example_create_user()

        if access_key:
            # Validate the key
            example_validate_key(access_key)

        # List all users
        example_list_users()

        print("\n" + "=" * 50)
        print("Examples completed successfully!")

    except requests.exceptions.ConnectionError:
        print("\n✗ Error: Cannot connect to the server.")
        print("Make sure the Flask backend is running on http://localhost:5000")
    except Exception as e:
        print(f"\n✗ Error: {str(e)}")

if __name__ == "__main__":
    main()
