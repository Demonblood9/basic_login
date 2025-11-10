#!/usr/bin/env python3
"""
Database initialization script for the license server.
Run this to create/update database tables.
"""

import sqlite3
import os

def init_database():
    """Initialize or update the database with all required tables"""

    db_path = os.path.join(os.path.dirname(__file__), 'license.db')
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    print("Checking database tables...")

    # Check if app_version table exists
    cursor.execute("SELECT name FROM sqlite_master WHERE type='table' AND name='app_version';")
    if not cursor.fetchone():
        print("Creating app_version table...")
        cursor.execute("""
            CREATE TABLE app_version (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                version VARCHAR(20) NOT NULL,
                is_current BOOLEAN DEFAULT 0,
                filename VARCHAR(255) NOT NULL,
                file_size INTEGER NOT NULL,
                release_notes TEXT,
                uploaded_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                uploaded_by VARCHAR(80) NOT NULL,
                download_count INTEGER DEFAULT 0
            )
        """)
        conn.commit()
        print("✓ app_version table created successfully")
    else:
        print("✓ app_version table already exists")

    # Show current state
    cursor.execute("SELECT COUNT(*) FROM app_version")
    count = cursor.fetchone()[0]
    print(f"\nCurrent versions in database: {count}")

    if count > 0:
        cursor.execute("SELECT id, version, is_current, filename FROM app_version")
        for row in cursor.fetchall():
            current_marker = " [CURRENT]" if row[2] else ""
            print(f"  - Version {row[1]}: {row[3]}{current_marker}")

    conn.close()
    print("\nDatabase initialization complete!")

if __name__ == '__main__':
    init_database()
