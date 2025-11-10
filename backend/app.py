from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime
import secrets
import hashlib
import os

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///keys.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SECRET_KEY'] = os.environ.get('SECRET_KEY', secrets.token_hex(32))

db = SQLAlchemy(app)

# Database Models
class User(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    access_key = db.Column(db.String(64), unique=True, nullable=False)
    access_key_hash = db.Column(db.String(64), nullable=False)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    last_login = db.Column(db.DateTime)
    is_active = db.Column(db.Boolean, default=True)

    def __repr__(self):
        return f'<User {self.username}>'

def hash_key(key):
    """Hash the access key using SHA-256"""
    return hashlib.sha256(key.encode()).hexdigest()

def generate_access_key():
    """Generate a cryptographically secure access key"""
    return secrets.token_urlsafe(32)

# API Endpoints
@app.route('/api/validate', methods=['POST'])
def validate_key():
    """Validate an access key"""
    data = request.get_json()

    if not data or 'key' not in data:
        return jsonify({'success': False, 'message': 'No key provided'}), 400

    key = data['key']
    key_hash = hash_key(key)

    user = User.query.filter_by(access_key_hash=key_hash).first()

    if user and user.is_active:
        user.last_login = datetime.utcnow()
        db.session.commit()
        return jsonify({
            'success': True,
            'message': 'Authentication successful',
            'username': user.username,
            'last_login': user.last_login.isoformat()
        }), 200
    else:
        return jsonify({'success': False, 'message': 'Invalid or inactive key'}), 401

@app.route('/api/admin/create_user', methods=['POST'])
def create_user():
    """Create a new user with an access key (Admin endpoint)"""
    data = request.get_json()

    if not data or 'username' not in data:
        return jsonify({'success': False, 'message': 'Username required'}), 400

    username = data['username']

    # Check if user already exists
    if User.query.filter_by(username=username).first():
        return jsonify({'success': False, 'message': 'Username already exists'}), 409

    # Generate access key
    access_key = generate_access_key()
    access_key_hash = hash_key(access_key)

    # Create new user
    new_user = User(
        username=username,
        access_key=access_key,  # Store plain key for initial retrieval
        access_key_hash=access_key_hash
    )

    db.session.add(new_user)
    db.session.commit()

    return jsonify({
        'success': True,
        'message': 'User created successfully',
        'username': username,
        'access_key': access_key,  # Return key only once
        'note': 'Save this key securely - it cannot be retrieved again'
    }), 201

@app.route('/api/admin/list_users', methods=['GET'])
def list_users():
    """List all users (Admin endpoint)"""
    users = User.query.all()
    return jsonify({
        'success': True,
        'users': [{
            'id': user.id,
            'username': user.username,
            'created_at': user.created_at.isoformat(),
            'last_login': user.last_login.isoformat() if user.last_login else None,
            'is_active': user.is_active
        } for user in users]
    }), 200

@app.route('/api/admin/deactivate_user/<int:user_id>', methods=['POST'])
def deactivate_user(user_id):
    """Deactivate a user (Admin endpoint)"""
    user = User.query.get(user_id)

    if not user:
        return jsonify({'success': False, 'message': 'User not found'}), 404

    user.is_active = False
    db.session.commit()

    return jsonify({'success': True, 'message': f'User {user.username} deactivated'}), 200

@app.route('/api/admin/activate_user/<int:user_id>', methods=['POST'])
def activate_user(user_id):
    """Activate a user (Admin endpoint)"""
    user = User.query.get(user_id)

    if not user:
        return jsonify({'success': False, 'message': 'User not found'}), 404

    user.is_active = True
    db.session.commit()

    return jsonify({'success': True, 'message': f'User {user.username} activated'}), 200

@app.route('/api/health', methods=['GET'])
def health_check():
    """Health check endpoint"""
    return jsonify({'status': 'healthy', 'timestamp': datetime.utcnow().isoformat()}), 200

# Initialize database
with app.app_context():
    db.create_all()
    print("Database initialized successfully")

if __name__ == '__main__':
    # For production, use a proper WSGI server like gunicorn
    # This is for development only
    app.run(host='0.0.0.0', port=5000, debug=True)
