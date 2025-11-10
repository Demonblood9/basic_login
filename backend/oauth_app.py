from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime, timedelta
import secrets
import hashlib
import os
import base64
import json

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///oauth.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SECRET_KEY'] = os.environ.get('SECRET_KEY', secrets.token_hex(32))

# OAuth 2.0 Configuration
ACCESS_TOKEN_EXPIRY = 3600  # 1 hour
REFRESH_TOKEN_EXPIRY = 2592000  # 30 days

db = SQLAlchemy(app)

# Database Models
class OAuthClient(db.Model):
    """OAuth 2.0 Client Application"""
    id = db.Column(db.Integer, primary_key=True)
    client_id = db.Column(db.String(64), unique=True, nullable=False)
    client_secret_hash = db.Column(db.String(64), nullable=False)
    client_name = db.Column(db.String(128), nullable=False)
    grant_types = db.Column(db.String(256), default='client_credentials')
    scopes = db.Column(db.String(256), default='read write')
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    is_active = db.Column(db.Boolean, default=True)

    def __repr__(self):
        return f'<OAuthClient {self.client_name}>'

class AccessToken(db.Model):
    """OAuth 2.0 Access Tokens"""
    id = db.Column(db.Integer, primary_key=True)
    token = db.Column(db.String(128), unique=True, nullable=False)
    token_hash = db.Column(db.String(64), nullable=False)
    client_id = db.Column(db.String(64), db.ForeignKey('oauth_client.client_id'), nullable=False)
    scope = db.Column(db.String(256))
    expires_at = db.Column(db.DateTime, nullable=False)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    revoked = db.Column(db.Boolean, default=False)

    def is_expired(self):
        return datetime.utcnow() > self.expires_at

    def __repr__(self):
        return f'<AccessToken {self.token[:10]}...>'

class RefreshToken(db.Model):
    """OAuth 2.0 Refresh Tokens"""
    id = db.Column(db.Integer, primary_key=True)
    token = db.Column(db.String(128), unique=True, nullable=False)
    token_hash = db.Column(db.String(64), nullable=False)
    client_id = db.Column(db.String(64), db.ForeignKey('oauth_client.client_id'), nullable=False)
    scope = db.Column(db.String(256))
    expires_at = db.Column(db.DateTime, nullable=False)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    revoked = db.Column(db.Boolean, default=False)

    def is_expired(self):
        return datetime.utcnow() > self.expires_at

    def __repr__(self):
        return f'<RefreshToken {self.token[:10]}...>'

# Helper Functions
def hash_secret(secret):
    """Hash client secret or token using SHA-256"""
    return hashlib.sha256(secret.encode()).hexdigest()

def generate_client_id():
    """Generate a unique client ID"""
    return secrets.token_urlsafe(24)

def generate_client_secret():
    """Generate a secure client secret"""
    return secrets.token_urlsafe(32)

def generate_access_token():
    """Generate a secure access token"""
    return secrets.token_urlsafe(48)

def generate_refresh_token():
    """Generate a secure refresh token"""
    return secrets.token_urlsafe(48)

def verify_client_credentials(client_id, client_secret):
    """Verify client credentials"""
    client = OAuthClient.query.filter_by(client_id=client_id, is_active=True).first()
    if not client:
        return None

    secret_hash = hash_secret(client_secret)
    if secret_hash == client.client_secret_hash:
        return client
    return None

def create_tokens(client_id, scope='read write'):
    """Create access and refresh tokens"""
    # Generate tokens
    access_token = generate_access_token()
    refresh_token_str = generate_refresh_token()

    # Calculate expiry times
    access_expires = datetime.utcnow() + timedelta(seconds=ACCESS_TOKEN_EXPIRY)
    refresh_expires = datetime.utcnow() + timedelta(seconds=REFRESH_TOKEN_EXPIRY)

    # Store access token
    access_token_obj = AccessToken(
        token=access_token,
        token_hash=hash_secret(access_token),
        client_id=client_id,
        scope=scope,
        expires_at=access_expires
    )
    db.session.add(access_token_obj)

    # Store refresh token
    refresh_token_obj = RefreshToken(
        token=refresh_token_str,
        token_hash=hash_secret(refresh_token_str),
        client_id=client_id,
        scope=scope,
        expires_at=refresh_expires
    )
    db.session.add(refresh_token_obj)

    db.session.commit()

    return {
        'access_token': access_token,
        'token_type': 'Bearer',
        'expires_in': ACCESS_TOKEN_EXPIRY,
        'refresh_token': refresh_token_str,
        'scope': scope
    }

# OAuth 2.0 Endpoints

@app.route('/oauth/token', methods=['POST'])
def token():
    """OAuth 2.0 Token Endpoint

    Supports:
    - client_credentials grant
    - refresh_token grant
    """
    grant_type = request.form.get('grant_type')

    if grant_type == 'client_credentials':
        # Client Credentials Grant
        client_id = request.form.get('client_id')
        client_secret = request.form.get('client_secret')
        scope = request.form.get('scope', 'read write')

        if not client_id or not client_secret:
            return jsonify({
                'error': 'invalid_request',
                'error_description': 'Missing client credentials'
            }), 400

        # Verify client
        client = verify_client_credentials(client_id, client_secret)
        if not client:
            return jsonify({
                'error': 'invalid_client',
                'error_description': 'Invalid client credentials'
            }), 401

        # Check if grant type is allowed
        if 'client_credentials' not in client.grant_types:
            return jsonify({
                'error': 'unauthorized_client',
                'error_description': 'Client not authorized for this grant type'
            }), 400

        # Create tokens
        tokens = create_tokens(client_id, scope)
        return jsonify(tokens), 200

    elif grant_type == 'refresh_token':
        # Refresh Token Grant
        refresh_token_str = request.form.get('refresh_token')
        client_id = request.form.get('client_id')
        client_secret = request.form.get('client_secret')

        if not refresh_token_str:
            return jsonify({
                'error': 'invalid_request',
                'error_description': 'Missing refresh token'
            }), 400

        # Verify client
        client = verify_client_credentials(client_id, client_secret)
        if not client:
            return jsonify({
                'error': 'invalid_client',
                'error_description': 'Invalid client credentials'
            }), 401

        # Find refresh token
        refresh_token_hash = hash_secret(refresh_token_str)
        refresh_token_obj = RefreshToken.query.filter_by(
            token_hash=refresh_token_hash,
            client_id=client_id,
            revoked=False
        ).first()

        if not refresh_token_obj or refresh_token_obj.is_expired():
            return jsonify({
                'error': 'invalid_grant',
                'error_description': 'Invalid or expired refresh token'
            }), 400

        # Revoke old refresh token
        refresh_token_obj.revoked = True
        db.session.commit()

        # Create new tokens
        tokens = create_tokens(client_id, refresh_token_obj.scope)
        return jsonify(tokens), 200

    else:
        return jsonify({
            'error': 'unsupported_grant_type',
            'error_description': f'Grant type {grant_type} is not supported'
        }), 400

@app.route('/oauth/introspect', methods=['POST'])
def introspect():
    """Token Introspection Endpoint (RFC 7662)"""
    token = request.form.get('token')

    if not token:
        return jsonify({'active': False}), 200

    # Check access token
    token_hash = hash_secret(token)
    access_token = AccessToken.query.filter_by(token_hash=token_hash, revoked=False).first()

    if access_token and not access_token.is_expired():
        return jsonify({
            'active': True,
            'client_id': access_token.client_id,
            'scope': access_token.scope,
            'exp': int(access_token.expires_at.timestamp()),
            'iat': int(access_token.created_at.timestamp())
        }), 200

    return jsonify({'active': False}), 200

@app.route('/oauth/revoke', methods=['POST'])
def revoke():
    """Token Revocation Endpoint (RFC 7009)"""
    token = request.form.get('token')
    token_type_hint = request.form.get('token_type_hint')

    if not token:
        return jsonify({'error': 'invalid_request'}), 400

    token_hash = hash_secret(token)

    # Try to revoke access token
    if token_type_hint != 'refresh_token':
        access_token = AccessToken.query.filter_by(token_hash=token_hash).first()
        if access_token:
            access_token.revoked = True
            db.session.commit()
            return '', 200

    # Try to revoke refresh token
    if token_type_hint != 'access_token':
        refresh_token = RefreshToken.query.filter_by(token_hash=token_hash).first()
        if refresh_token:
            refresh_token.revoked = True
            db.session.commit()
            return '', 200

    # Token not found (still return success per RFC 7009)
    return '', 200

# Protected Resource Endpoint
@app.route('/api/protected', methods=['GET'])
def protected_resource():
    """Example protected resource that requires valid access token"""
    auth_header = request.headers.get('Authorization')

    if not auth_header or not auth_header.startswith('Bearer '):
        return jsonify({
            'error': 'invalid_token',
            'error_description': 'Missing or invalid authorization header'
        }), 401

    token = auth_header[7:]  # Remove 'Bearer ' prefix
    token_hash = hash_secret(token)

    # Validate token
    access_token = AccessToken.query.filter_by(token_hash=token_hash, revoked=False).first()

    if not access_token or access_token.is_expired():
        return jsonify({
            'error': 'invalid_token',
            'error_description': 'Token is invalid or expired'
        }), 401

    # Token is valid
    client = OAuthClient.query.filter_by(client_id=access_token.client_id).first()

    return jsonify({
        'success': True,
        'message': 'Access granted',
        'client_name': client.client_name if client else 'Unknown',
        'scope': access_token.scope,
        'expires_at': access_token.expires_at.isoformat()
    }), 200

# Admin Endpoints
@app.route('/admin/clients', methods=['POST'])
def create_client():
    """Create a new OAuth 2.0 client"""
    data = request.get_json()

    client_name = data.get('client_name')
    grant_types = data.get('grant_types', 'client_credentials refresh_token')
    scopes = data.get('scopes', 'read write')

    if not client_name:
        return jsonify({'error': 'client_name required'}), 400

    # Generate credentials
    client_id = generate_client_id()
    client_secret = generate_client_secret()

    # Create client
    client = OAuthClient(
        client_id=client_id,
        client_secret_hash=hash_secret(client_secret),
        client_name=client_name,
        grant_types=grant_types,
        scopes=scopes
    )

    db.session.add(client)
    db.session.commit()

    return jsonify({
        'success': True,
        'client_id': client_id,
        'client_secret': client_secret,
        'client_name': client_name,
        'grant_types': grant_types,
        'scopes': scopes,
        'note': 'Save the client_secret securely - it cannot be retrieved again'
    }), 201

@app.route('/admin/clients', methods=['GET'])
def list_clients():
    """List all OAuth 2.0 clients"""
    clients = OAuthClient.query.all()

    return jsonify({
        'success': True,
        'clients': [{
            'id': client.id,
            'client_id': client.client_id,
            'client_name': client.client_name,
            'grant_types': client.grant_types,
            'scopes': client.scopes,
            'is_active': client.is_active,
            'created_at': client.created_at.isoformat()
        } for client in clients]
    }), 200

@app.route('/admin/clients/<int:client_db_id>', methods=['DELETE'])
def delete_client(client_db_id):
    """Deactivate an OAuth 2.0 client"""
    client = OAuthClient.query.get(client_db_id)

    if not client:
        return jsonify({'error': 'Client not found'}), 404

    client.is_active = False
    db.session.commit()

    return jsonify({'success': True, 'message': f'Client {client.client_name} deactivated'}), 200

@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.utcnow().isoformat(),
        'oauth_version': '2.0'
    }), 200

# Initialize database
with app.app_context():
    db.create_all()
    print("OAuth 2.0 Database initialized successfully")

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
