#!/usr/bin/env python3
"""
Create Admin User Script
Creates an admin user for accessing the web admin panel
"""

from license_server import app, db, AdminUser
import getpass

def create_admin():
    """Create a new admin user"""
    print("=" * 50)
    print("  Create Admin User - License Manager")
    print("=" * 50)
    print()

    with app.app_context():
        # Check if any admin users exist
        existing_admins = AdminUser.query.count()
        if existing_admins > 0:
            print(f"⚠️  Warning: {existing_admins} admin user(s) already exist")
            print()
            response = input("Do you want to create another admin? (yes/no): ").strip().lower()
            if response not in ['yes', 'y']:
                print("Cancelled.")
                return

        print()
        print("Enter admin credentials:")
        print("-" * 50)

        # Get username
        while True:
            username = input("Username: ").strip()
            if not username:
                print("❌ Username cannot be empty")
                continue

            # Check if username already exists
            existing = AdminUser.query.filter_by(username=username).first()
            if existing:
                print(f"❌ Username '{username}' already exists")
                continue

            break

        # Get password
        while True:
            password = getpass.getpass("Password: ")
            if len(password) < 6:
                print("❌ Password must be at least 6 characters")
                continue

            password_confirm = getpass.getpass("Confirm password: ")
            if password != password_confirm:
                print("❌ Passwords do not match")
                continue

            break

        # Create admin user
        admin = AdminUser(username=username)
        admin.set_password(password)

        db.session.add(admin)
        db.session.commit()

        print()
        print("=" * 50)
        print("✅ Admin user created successfully!")
        print("=" * 50)
        print()
        print(f"Username: {username}")
        print(f"Password: {'*' * len(password)}")
        print()
        print("You can now log in at:")
        print("  http://localhost:5000/admin/login")
        print()

if __name__ == '__main__':
    create_admin()
