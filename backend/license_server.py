from flask import Flask, request, jsonify
from flask_sqlalchemy import SQLAlchemy
from datetime import datetime, timedelta
import secrets
import hashlib
import os

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///license.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SECRET_KEY'] = os.environ.get('SECRET_KEY', secrets.token_hex(32))

db = SQLAlchemy(app)

# Database Models
class LicenseKey(db.Model):
    """License keys with HWID binding and expiration"""
    id = db.Column(db.Integer, primary_key=True)
    license_key = db.Column(db.String(64), unique=True, nullable=False)
    key_hash = db.Column(db.String(64), nullable=False)
    username = db.Column(db.String(128), nullable=False)
    hwid = db.Column(db.String(128), nullable=True)  # Hardware ID binding
    ip_address = db.Column(db.String(45), nullable=True)  # Last known IP
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    expires_at = db.Column(db.DateTime, nullable=True)  # None = permanent
    last_login = db.Column(db.DateTime)
    is_active = db.Column(db.Boolean, default=True)
    is_suspended = db.Column(db.Boolean, default=False)
    suspension_reason = db.Column(db.String(256))

    # Relationships
    login_history = db.relationship('LoginHistory', backref='license', lazy=True, cascade='all, delete-orphan')

    def is_expired(self):
        if self.expires_at is None:
            return False
        return datetime.utcnow() > self.expires_at

    def days_remaining(self):
        if self.expires_at is None:
            return -1  # Permanent
        delta = self.expires_at - datetime.utcnow()
        return max(0, delta.days)

    def __repr__(self):
        return f'<LicenseKey {self.username}>'

class LoginHistory(db.Model):
    """Login attempt history with IP and HWID tracking"""
    id = db.Column(db.Integer, primary_key=True)
    license_id = db.Column(db.Integer, db.ForeignKey('license_key.id'), nullable=False)
    ip_address = db.Column(db.String(45), nullable=False)
    hwid = db.Column(db.String(128), nullable=False)
    timestamp = db.Column(db.DateTime, default=datetime.utcnow)
    success = db.Column(db.Boolean, default=True)
    failure_reason = db.Column(db.String(256))

    def __repr__(self):
        return f'<LoginHistory {self.ip_address} @ {self.timestamp}>'

# Helper Functions
def hash_key(key):
    """Hash license key using SHA-256"""
    return hashlib.sha256(key.encode()).hexdigest()

def generate_license_key():
    """Generate a cryptographically secure license key"""
    return secrets.token_urlsafe(32)

def get_client_ip():
    """Get client IP address from request"""
    if request.headers.get('X-Forwarded-For'):
        return request.headers.get('X-Forwarded-For').split(',')[0]
    return request.remote_addr

# API Endpoints

@app.route('/api/validate', methods=['POST'])
def validate_license():
    """Validate license key with HWID binding"""
    data = request.get_json()

    if not data or 'key' not in data or 'hwid' not in data:
        return jsonify({
            'success': False,
            'message': 'License key and HWID required'
        }), 400

    key = data['key']
    hwid = data['hwid']
    ip_address = get_client_ip()

    key_hash = hash_key(key)
    license_key = LicenseKey.query.filter_by(key_hash=key_hash).first()

    # Record login attempt
    def record_login(license_obj, success, reason=None):
        history = LoginHistory(
            license_id=license_obj.id if license_obj else None,
            ip_address=ip_address,
            hwid=hwid,
            success=success,
            failure_reason=reason
        )
        if license_obj:
            db.session.add(history)
            db.session.commit()

    # Check if license exists
    if not license_key:
        return jsonify({
            'success': False,
            'message': 'Invalid license key'
        }), 401

    # Check if suspended
    if license_key.is_suspended:
        record_login(license_key, False, f'Suspended: {license_key.suspension_reason}')
        return jsonify({
            'success': False,
            'message': f'License suspended: {license_key.suspension_reason}'
        }), 403

    # Check if expired
    if license_key.is_expired():
        record_login(license_key, False, 'License expired')
        return jsonify({
            'success': False,
            'message': 'License has expired'
        }), 403

    # Check if inactive
    if not license_key.is_active:
        record_login(license_key, False, 'License inactive')
        return jsonify({
            'success': False,
            'message': 'License is inactive'
        }), 403

    # HWID Binding Check
    if license_key.hwid is None:
        # First login - bind HWID
        license_key.hwid = hwid
        license_key.ip_address = ip_address
        license_key.last_login = datetime.utcnow()
        db.session.commit()
    elif license_key.hwid != hwid:
        # HWID mismatch - auto-suspend
        license_key.is_suspended = True
        license_key.suspension_reason = f'HWID violation detected. Registered: {license_key.hwid[:16]}..., Attempted: {hwid[:16]}...'
        db.session.commit()

        record_login(license_key, False, license_key.suspension_reason)

        return jsonify({
            'success': False,
            'message': 'HWID mismatch detected. License has been automatically suspended for security.',
            'details': 'This key is registered to another machine. Contact administrator.'
        }), 403

    # Update last login
    license_key.last_login = datetime.utcnow()
    license_key.ip_address = ip_address
    db.session.commit()

    # Record successful login
    record_login(license_key, True)

    # Return success
    return jsonify({
        'success': True,
        'message': 'Authentication successful',
        'username': license_key.username,
        'expires_at': license_key.expires_at.isoformat() if license_key.expires_at else 'Never',
        'days_remaining': license_key.days_remaining(),
        'is_permanent': license_key.expires_at is None
    }), 200

# Admin Endpoints

@app.route('/api/admin/licenses', methods=['POST'])
def create_license():
    """Create a new license key"""
    data = request.get_json()

    username = data.get('username')
    days = data.get('days', 0)  # 0 = permanent

    if not username:
        return jsonify({'success': False, 'message': 'Username required'}), 400

    # Check if username already has a license
    existing = LicenseKey.query.filter_by(username=username).first()
    if existing:
        return jsonify({
            'success': False,
            'message': f'License already exists for {username}'
        }), 409

    # Generate license key
    license_key = generate_license_key()

    # Calculate expiration
    expires_at = None
    if days > 0:
        expires_at = datetime.utcnow() + timedelta(days=days)

    # Create license
    new_license = LicenseKey(
        license_key=license_key,
        key_hash=hash_key(license_key),
        username=username,
        expires_at=expires_at
    )

    db.session.add(new_license)
    db.session.commit()

    return jsonify({
        'success': True,
        'license_key': license_key,
        'username': username,
        'expires_at': expires_at.isoformat() if expires_at else 'Never',
        'days': days if days > 0 else 'Permanent',
        'note': 'Save this license key securely - it cannot be retrieved again'
    }), 201

@app.route('/api/admin/licenses', methods=['GET'])
def list_licenses():
    """List all license keys"""
    licenses = LicenseKey.query.all()

    return jsonify({
        'success': True,
        'licenses': [{
            'id': lic.id,
            'username': lic.username,
            'hwid': lic.hwid[:16] + '...' if lic.hwid else 'Not bound',
            'ip_address': lic.ip_address,
            'created_at': lic.created_at.isoformat(),
            'expires_at': lic.expires_at.isoformat() if lic.expires_at else 'Never',
            'days_remaining': lic.days_remaining(),
            'last_login': lic.last_login.isoformat() if lic.last_login else 'Never',
            'is_active': lic.is_active,
            'is_suspended': lic.is_suspended,
            'suspension_reason': lic.suspension_reason,
            'is_expired': lic.is_expired()
        } for lic in licenses]
    }), 200

@app.route('/api/admin/licenses/<int:license_id>', methods=['GET'])
def get_license_details(license_id):
    """Get detailed license information including login history"""
    license_key = LicenseKey.query.get(license_id)

    if not license_key:
        return jsonify({'success': False, 'message': 'License not found'}), 404

    # Get login history
    history = LoginHistory.query.filter_by(license_id=license_id)\
        .order_by(LoginHistory.timestamp.desc())\
        .limit(100)\
        .all()

    return jsonify({
        'success': True,
        'license': {
            'id': license_key.id,
            'username': license_key.username,
            'hwid': license_key.hwid,
            'ip_address': license_key.ip_address,
            'created_at': license_key.created_at.isoformat(),
            'expires_at': license_key.expires_at.isoformat() if license_key.expires_at else 'Never',
            'days_remaining': license_key.days_remaining(),
            'last_login': license_key.last_login.isoformat() if license_key.last_login else 'Never',
            'is_active': license_key.is_active,
            'is_suspended': license_key.is_suspended,
            'suspension_reason': license_key.suspension_reason,
            'is_expired': license_key.is_expired()
        },
        'login_history': [{
            'timestamp': h.timestamp.isoformat(),
            'ip_address': h.ip_address,
            'hwid': h.hwid[:16] + '...' if h.hwid else 'Unknown',
            'success': h.success,
            'failure_reason': h.failure_reason
        } for h in history]
    }), 200

@app.route('/api/admin/licenses/<int:license_id>/suspend', methods=['POST'])
def suspend_license(license_id):
    """Suspend a license"""
    data = request.get_json()
    reason = data.get('reason', 'Suspended by administrator')

    license_key = LicenseKey.query.get(license_id)

    if not license_key:
        return jsonify({'success': False, 'message': 'License not found'}), 404

    license_key.is_suspended = True
    license_key.suspension_reason = reason
    db.session.commit()

    return jsonify({
        'success': True,
        'message': f'License for {license_key.username} suspended'
    }), 200

@app.route('/api/admin/licenses/<int:license_id>/unsuspend', methods=['POST'])
def unsuspend_license(license_id):
    """Unsuspend a license"""
    license_key = LicenseKey.query.get(license_id)

    if not license_key:
        return jsonify({'success': False, 'message': 'License not found'}), 404

    license_key.is_suspended = False
    license_key.suspension_reason = None
    db.session.commit()

    return jsonify({
        'success': True,
        'message': f'License for {license_key.username} unsuspended'
    }), 200

@app.route('/api/admin/licenses/<int:license_id>/reset-hwid', methods=['POST'])
def reset_hwid(license_id):
    """Reset HWID binding (useful for hardware changes)"""
    license_key = LicenseKey.query.get(license_id)

    if not license_key:
        return jsonify({'success': False, 'message': 'License not found'}), 404

    old_hwid = license_key.hwid
    license_key.hwid = None
    license_key.is_suspended = False
    license_key.suspension_reason = None
    db.session.commit()

    return jsonify({
        'success': True,
        'message': f'HWID reset for {license_key.username}',
        'old_hwid': old_hwid
    }), 200

@app.route('/api/admin/licenses/<int:license_id>', methods=['DELETE'])
def delete_license(license_id):
    """Delete a license"""
    license_key = LicenseKey.query.get(license_id)

    if not license_key:
        return jsonify({'success': False, 'message': 'License not found'}), 404

    username = license_key.username
    db.session.delete(license_key)
    db.session.commit()

    return jsonify({
        'success': True,
        'message': f'License for {username} deleted'
    }), 200

@app.route('/api/admin/licenses/<int:license_id>/extend', methods=['POST'])
def extend_license(license_id):
    """Extend license expiration"""
    data = request.get_json()
    days = data.get('days', 30)

    license_key = LicenseKey.query.get(license_id)

    if not license_key:
        return jsonify({'success': False, 'message': 'License not found'}), 404

    if license_key.expires_at:
        # Extend from current expiration
        if license_key.is_expired():
            license_key.expires_at = datetime.utcnow() + timedelta(days=days)
        else:
            license_key.expires_at += timedelta(days=days)
    else:
        # Was permanent, make it expiring
        license_key.expires_at = datetime.utcnow() + timedelta(days=days)

    db.session.commit()

    return jsonify({
        'success': True,
        'message': f'License extended by {days} days',
        'new_expiration': license_key.expires_at.isoformat(),
        'days_remaining': license_key.days_remaining()
    }), 200

@app.route('/api/admin/stats', methods=['GET'])
def get_stats():
    """Get system statistics"""
    total_licenses = LicenseKey.query.count()
    active_licenses = LicenseKey.query.filter_by(is_active=True, is_suspended=False).count()
    suspended_licenses = LicenseKey.query.filter_by(is_suspended=True).count()
    expired_licenses = LicenseKey.query.filter(LicenseKey.expires_at < datetime.utcnow()).count()

    # Recent logins (last 24 hours)
    yesterday = datetime.utcnow() - timedelta(days=1)
    recent_logins = LoginHistory.query.filter(LoginHistory.timestamp >= yesterday).count()

    return jsonify({
        'success': True,
        'stats': {
            'total_licenses': total_licenses,
            'active_licenses': active_licenses,
            'suspended_licenses': suspended_licenses,
            'expired_licenses': expired_licenses,
            'logins_24h': recent_logins
        }
    }), 200

@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({
        'status': 'healthy',
        'timestamp': datetime.utcnow().isoformat()
    }), 200

# Initialize database
with app.app_context():
    db.create_all()
    print("License Management Database initialized successfully")

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
