from flask import Flask, request, jsonify, render_template, redirect, url_for, flash, session, send_file
from flask_sqlalchemy import SQLAlchemy
from functools import wraps
from datetime import datetime, timedelta
from werkzeug.utils import secure_filename
import secrets
import hashlib
import os

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = 'sqlite:///license.db'
app.config['SQLALCHEMY_TRACK_MODIFICATIONS'] = False
app.config['SECRET_KEY'] = os.environ.get('SECRET_KEY', secrets.token_hex(32))

# Upload configuration
UPLOAD_FOLDER = os.path.join(os.path.dirname(__file__), 'updates')
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
app.config['MAX_CONTENT_LENGTH'] = 100 * 1024 * 1024  # 100MB max file size

# Create upload folder if it doesn't exist
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

db = SQLAlchemy(app)

# Database Models
class LicenseKey(db.Model):
    """License keys with HWID binding and expiration"""
    id = db.Column(db.Integer, primary_key=True)
    license_key = db.Column(db.String(64), unique=True, nullable=False)
    key_hash = db.Column(db.String(64), nullable=False)
    username = db.Column(db.String(128), nullable=False)
    hwid = db.Column(db.String(128), nullable=True)
    ip_address = db.Column(db.String(45), nullable=True)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    expires_at = db.Column(db.DateTime, nullable=True)
    last_login = db.Column(db.DateTime)
    is_active = db.Column(db.Boolean, default=True)
    is_suspended = db.Column(db.Boolean, default=False)
    suspension_reason = db.Column(db.String(256))

    login_history = db.relationship('LoginHistory', backref='license', lazy=True, cascade='all, delete-orphan')

    def is_expired(self):
        if self.expires_at is None:
            return False
        return datetime.utcnow() > self.expires_at

    def days_remaining(self):
        if self.expires_at is None:
            return -1
        delta = self.expires_at - datetime.utcnow()
        return max(0, delta.days)

    def time_remaining(self):
        """Returns time remaining formatted as '2 Days 16 Hours 23 Minutes'"""
        if self.expires_at is None:
            return 'Permanent'

        delta = self.expires_at - datetime.utcnow()
        if delta.total_seconds() <= 0:
            return 'Expired'

        days = delta.days
        hours = delta.seconds // 3600
        minutes = (delta.seconds % 3600) // 60

        parts = []
        if days > 0:
            parts.append(f"{days} Day{'s' if days != 1 else ''}")
        if hours > 0:
            parts.append(f"{hours} Hour{'s' if hours != 1 else ''}")
        if minutes > 0 or not parts:  # Show minutes if it's the only value
            parts.append(f"{minutes} Minute{'s' if minutes != 1 else ''}")

        return ' '.join(parts)

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

class AdminUser(db.Model):
    """Admin users for accessing the admin panel"""
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password_hash = db.Column(db.String(128), nullable=False)
    created_at = db.Column(db.DateTime, default=datetime.utcnow)
    last_login = db.Column(db.DateTime)

    def set_password(self, password):
        """Hash and set password"""
        self.password_hash = hashlib.sha256(password.encode()).hexdigest()

    def check_password(self, password):
        """Verify password"""
        return self.password_hash == hashlib.sha256(password.encode()).hexdigest()

    def __repr__(self):
        return f'<AdminUser {self.username}>'

class AppVersion(db.Model):
    """Application version management for auto-updates"""
    id = db.Column(db.Integer, primary_key=True)
    version = db.Column(db.String(20), nullable=False)  # e.g., "1.0.0"
    is_current = db.Column(db.Boolean, default=False)  # Only one should be True
    filename = db.Column(db.String(255), nullable=False)  # Stored zip filename
    file_size = db.Column(db.Integer, nullable=False)  # File size in bytes
    release_notes = db.Column(db.Text, nullable=True)
    uploaded_at = db.Column(db.DateTime, default=datetime.utcnow)
    uploaded_by = db.Column(db.String(80), nullable=False)
    download_count = db.Column(db.Integer, default=0)

    def __repr__(self):
        return f'<AppVersion {self.version}>'

# Helper Functions
def hash_key(key):
    return hashlib.sha256(key.encode()).hexdigest()

def generate_license_key():
    return secrets.token_urlsafe(32)

def get_client_ip():
    if request.headers.get('X-Forwarded-For'):
        return request.headers.get('X-Forwarded-For').split(',')[0]
    return request.remote_addr

# Custom Jinja2 filter for date formatting
@app.template_filter('format_datetime')
def format_datetime(value):
    """Format datetime as 'December 12 2025 - 09:00 (EST)'"""
    if value is None:
        return 'Never'
    if isinstance(value, str):
        return value  # Already formatted
    return value.strftime('%B %d %Y - %H:%M (EST)')

# Authentication Decorator
def login_required(f):
    """Decorator to require admin login for routes"""
    @wraps(f)
    def decorated_function(*args, **kwargs):
        if 'admin_logged_in' not in session:
            flash('Please log in to access the admin panel', 'warning')
            return redirect(url_for('admin_login'))
        return f(*args, **kwargs)
    return decorated_function

# ============================================================================
# AUTHENTICATION ROUTES
# ============================================================================

@app.route('/admin/login', methods=['GET', 'POST'])
def admin_login():
    """Admin login page"""
    if 'admin_logged_in' in session:
        return redirect(url_for('admin_dashboard'))

    if request.method == 'POST':
        username = request.form.get('username')
        password = request.form.get('password')

        admin = AdminUser.query.filter_by(username=username).first()

        if admin and admin.check_password(password):
            session['admin_logged_in'] = True
            session['admin_username'] = username
            admin.last_login = datetime.utcnow()
            db.session.commit()
            flash(f'Welcome back, {username}!', 'success')
            return redirect(url_for('admin_dashboard'))
        else:
            flash('Invalid username or password', 'danger')

    return render_template('admin_login.html')

@app.route('/admin/logout')
def admin_logout():
    """Logout admin user"""
    session.pop('admin_logged_in', None)
    session.pop('admin_username', None)
    flash('You have been logged out successfully', 'info')
    return redirect(url_for('admin_login'))

# ============================================================================
# WEB ADMIN PANEL ROUTES
# ============================================================================

@app.route('/')
@login_required
def index():
    """Redirect to admin dashboard"""
    return redirect(url_for('admin_dashboard'))

@app.route('/admin')
@app.route('/admin/dashboard')
@login_required
def admin_dashboard():
    """Admin dashboard with statistics"""
    total_licenses = LicenseKey.query.count()
    active_licenses = LicenseKey.query.filter_by(is_active=True, is_suspended=False).count()
    suspended_licenses = LicenseKey.query.filter_by(is_suspended=True).count()

    # Count expired licenses
    expired_count = 0
    for lic in LicenseKey.query.all():
        if lic.is_expired():
            expired_count += 1

    # Recent logins (last 24 hours)
    yesterday = datetime.utcnow() - timedelta(days=1)
    recent_logins = LoginHistory.query.filter(LoginHistory.timestamp >= yesterday).count()

    # Recent licenses
    recent_licenses = LicenseKey.query.order_by(LicenseKey.created_at.desc()).limit(5).all()

    return render_template('dashboard.html',
                         total_licenses=total_licenses,
                         active_licenses=active_licenses,
                         suspended_licenses=suspended_licenses,
                         expired_licenses=expired_count,
                         recent_logins=recent_logins,
                         recent_licenses=recent_licenses)

@app.route('/admin/licenses')
@login_required
def admin_licenses():
    """List all licenses"""
    licenses = LicenseKey.query.order_by(LicenseKey.created_at.desc()).all()
    return render_template('licenses.html', licenses=licenses)

@app.route('/admin/licenses/create', methods=['GET', 'POST'])
@login_required
def admin_create_license():
    """Create new license"""
    if request.method == 'POST':
        username = request.form.get('username')
        days_str = request.form.get('days', '0')

        # Handle custom days
        if days_str == 'custom':
            days = int(request.form.get('customDays', 0))
        else:
            days = int(days_str)

        if not username:
            flash('Username is required', 'danger')
            return redirect(url_for('admin_create_license'))

        # Check if username exists
        existing = LicenseKey.query.filter_by(username=username).first()
        if existing:
            flash(f'License already exists for {username}', 'warning')
            return redirect(url_for('admin_licenses'))

        # Generate license
        license_key = generate_license_key()
        expires_at = None if days == 0 else datetime.utcnow() + timedelta(days=days)

        new_license = LicenseKey(
            license_key=license_key,
            key_hash=hash_key(license_key),
            username=username,
            expires_at=expires_at
        )

        db.session.add(new_license)
        db.session.commit()

        flash(f'License created for {username}!', 'success')
        return render_template('license_created.html',
                             username=username,
                             license_key=license_key,
                             created_at=new_license.created_at.strftime('%Y-%m-%d %H:%M'),
                             expires_at=new_license.expires_at.strftime('%Y-%m-%d %H:%M') if new_license.expires_at else None,
                             days=days)

    return render_template('create_license.html')

@app.route('/admin/licenses/<int:license_id>')
@login_required
def admin_license_details(license_id):
    """View license details and history"""
    license_key = LicenseKey.query.get_or_404(license_id)
    history = LoginHistory.query.filter_by(license_id=license_id)\
        .order_by(LoginHistory.timestamp.desc())\
        .limit(100)\
        .all()

    return render_template('license_details.html', license=license_key, login_history=history)

@app.route('/admin/licenses/<int:license_id>/suspend', methods=['POST'])
@login_required
def admin_suspend_license(license_id):
    """Suspend a license"""
    license_key = LicenseKey.query.get_or_404(license_id)
    reason = request.form.get('reason', 'Suspended by administrator')

    license_key.is_suspended = True
    license_key.suspension_reason = reason
    db.session.commit()

    flash(f'License for {license_key.username} has been suspended', 'warning')
    return redirect(url_for('admin_license_details', license_id=license_id))

@app.route('/admin/licenses/<int:license_id>/unsuspend', methods=['POST'])
@login_required
def admin_unsuspend_license(license_id):
    """Unsuspend a license"""
    license_key = LicenseKey.query.get_or_404(license_id)

    license_key.is_suspended = False
    license_key.suspension_reason = None
    db.session.commit()

    flash(f'License for {license_key.username} has been unsuspended', 'success')
    return redirect(url_for('admin_license_details', license_id=license_id))

@app.route('/admin/licenses/<int:license_id>/reset-hwid', methods=['POST'])
@login_required
def admin_reset_hwid(license_id):
    """Reset HWID binding"""
    license_key = LicenseKey.query.get_or_404(license_id)

    old_hwid = license_key.hwid
    license_key.hwid = None
    license_key.is_suspended = False
    license_key.suspension_reason = None
    db.session.commit()

    flash(f'HWID reset for {license_key.username}. User can now bind to a new machine.', 'info')
    return redirect(url_for('admin_license_details', license_id=license_id))

@app.route('/admin/licenses/<int:license_id>/extend', methods=['POST'])
@login_required
def admin_extend_license(license_id):
    """Extend license expiration"""
    license_key = LicenseKey.query.get_or_404(license_id)
    days = int(request.form.get('days', 30))

    if license_key.expires_at:
        if license_key.is_expired():
            license_key.expires_at = datetime.utcnow() + timedelta(days=days)
        else:
            license_key.expires_at += timedelta(days=days)
    else:
        license_key.expires_at = datetime.utcnow() + timedelta(days=days)

    db.session.commit()

    flash(f'License extended by {days} days', 'success')
    return redirect(url_for('admin_license_details', license_id=license_id))

@app.route('/admin/licenses/<int:license_id>/delete', methods=['POST'])
@login_required
def admin_delete_license(license_id):
    """Delete a license"""
    license_key = LicenseKey.query.get_or_404(license_id)
    username = license_key.username

    db.session.delete(license_key)
    db.session.commit()

    flash(f'License for {username} has been deleted', 'danger')
    return redirect(url_for('admin_licenses'))

# ============================================================================
# VERSION MANAGEMENT ROUTES
# ============================================================================

@app.route('/admin/versions')
@login_required
def admin_versions():
    """List all app versions"""
    versions = AppVersion.query.order_by(AppVersion.uploaded_at.desc()).all()
    current_version = AppVersion.query.filter_by(is_current=True).first()

    # Calculate total disk space used
    total_size = sum(v.file_size for v in versions)
    total_downloads = sum(v.download_count for v in versions)

    return render_template('versions.html',
                         versions=versions,
                         current_version=current_version,
                         total_size=total_size,
                         total_downloads=total_downloads)

@app.route('/admin/versions/upload', methods=['GET', 'POST'])
@login_required
def admin_upload_version():
    """Upload new version"""
    if request.method == 'POST':
        version = request.form.get('version')
        release_notes = request.form.get('release_notes', '')

        if 'update_file' not in request.files:
            flash('No file uploaded', 'danger')
            return redirect(url_for('admin_upload_version'))

        file = request.files['update_file']

        if file.filename == '':
            flash('No file selected', 'danger')
            return redirect(url_for('admin_upload_version'))

        if not file.filename.endswith('.zip'):
            flash('Only ZIP files are allowed', 'danger')
            return redirect(url_for('admin_upload_version'))

        if not version:
            flash('Version number is required', 'danger')
            return redirect(url_for('admin_upload_version'))

        # Check if version already exists
        existing = AppVersion.query.filter_by(version=version).first()
        if existing:
            flash(f'Version {version} already exists', 'warning')
            return redirect(url_for('admin_versions'))

        # Save file
        filename = f"update_v{version}.zip"
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)

        # Get file size
        file_size = os.path.getsize(filepath)

        # Create version record
        new_version = AppVersion(
            version=version,
            filename=filename,
            file_size=file_size,
            release_notes=release_notes,
            uploaded_by=session.get('admin_username', 'admin'),
            is_current=False
        )

        db.session.add(new_version)
        db.session.commit()

        flash(f'Version {version} uploaded successfully! Set it as current to enable auto-updates.', 'success')
        return redirect(url_for('admin_versions'))

    return render_template('upload_version.html')

@app.route('/admin/versions/<int:version_id>/set-current', methods=['POST'])
@login_required
def admin_set_current_version(version_id):
    """Set a version as current"""
    version = AppVersion.query.get_or_404(version_id)

    # Unset all other versions
    AppVersion.query.update({'is_current': False})

    # Set this version as current
    version.is_current = True
    db.session.commit()

    flash(f'Version {version.version} is now the current version', 'success')
    return redirect(url_for('admin_versions'))

@app.route('/admin/versions/<int:version_id>/delete', methods=['POST'])
@login_required
def admin_delete_version(version_id):
    """Delete a version"""
    version = AppVersion.query.get_or_404(version_id)

    if version.is_current:
        flash('Cannot delete the current version. Set another version as current first.', 'danger')
        return redirect(url_for('admin_versions'))

    # Delete file
    filepath = os.path.join(app.config['UPLOAD_FOLDER'], version.filename)
    if os.path.exists(filepath):
        os.remove(filepath)

    version_num = version.version
    db.session.delete(version)
    db.session.commit()

    flash(f'Version {version_num} has been deleted', 'info')
    return redirect(url_for('admin_versions'))

# ============================================================================
# API ENDPOINTS (for client authentication)
# ============================================================================

@app.route('/api/validate', methods=['POST'])
def validate_license():
    """Validate license key with HWID binding"""
    data = request.get_json()

    # Debug logging
    print("\n=== DEBUG: Validate License ===")
    print(f"Received data: {data}")

    if not data or 'key' not in data or 'hwid' not in data:
        print("ERROR: Missing key or hwid in request")
        return jsonify({
            'success': False,
            'message': 'License key and HWID required'
        }), 400

    key = data['key']
    hwid = data['hwid']
    ip_address = get_client_ip()

    print(f"Key received: {key}")
    print(f"HWID received: {hwid}")
    print(f"IP address: {ip_address}")

    key_hash = hash_key(key)
    print(f"Key hash: {key_hash}")

    license_key = LicenseKey.query.filter_by(key_hash=key_hash).first()
    print(f"License found in DB: {license_key is not None}")

    def record_login(license_obj, success, reason=None):
        if license_obj:
            history = LoginHistory(
                license_id=license_obj.id,
                ip_address=ip_address,
                hwid=hwid,
                success=success,
                failure_reason=reason
            )
            db.session.add(history)
            db.session.commit()

    if not license_key:
        print("ERROR: License key not found in database")
        return jsonify({'success': False, 'message': 'Invalid license key'}), 401

    print(f"License details - Username: {license_key.username}, Active: {license_key.is_active}, Suspended: {license_key.is_suspended}")
    print(f"License HWID in DB: {license_key.hwid}")
    print(f"License expires: {license_key.expires_at}")

    if license_key.is_suspended:
        print(f"ERROR: License is suspended - {license_key.suspension_reason}")
        record_login(license_key, False, f'Suspended: {license_key.suspension_reason}')
        return jsonify({
            'success': False,
            'message': f'License suspended: {license_key.suspension_reason}'
        }), 403

    if license_key.is_expired():
        print("ERROR: License has expired")
        record_login(license_key, False, 'License expired')
        return jsonify({'success': False, 'message': 'License has expired'}), 403

    if not license_key.is_active:
        print("ERROR: License is not active")
        record_login(license_key, False, 'License inactive')
        return jsonify({'success': False, 'message': 'License is inactive'}), 403

    # HWID Binding Check
    if license_key.hwid is None:
        print("INFO: First time login - binding HWID")
        license_key.hwid = hwid
        license_key.ip_address = ip_address
        license_key.last_login = datetime.utcnow()
        db.session.commit()
    elif license_key.hwid != hwid:
        print(f"ERROR: HWID mismatch - DB: {license_key.hwid}, Client: {hwid}")
        license_key.is_suspended = True
        license_key.suspension_reason = f'HWID violation detected. Registered: {license_key.hwid[:16]}..., Attempted: {hwid[:16]}...'
        db.session.commit()
        record_login(license_key, False, license_key.suspension_reason)
        return jsonify({
            'success': False,
            'message': 'HWID mismatch detected. License has been automatically suspended for security.',
            'details': 'This key is registered to another machine. Contact administrator.'
        }), 403
    else:
        print("INFO: HWID matches")

    license_key.last_login = datetime.utcnow()
    license_key.ip_address = ip_address
    db.session.commit()

    record_login(license_key, True)

    print("SUCCESS: Authentication successful")

    # Format expiration date as "December 12 2025 - 09:00 (EST)"
    expires_formatted = 'Never'
    if license_key.expires_at:
        expires_formatted = license_key.expires_at.strftime('%B %d %Y - %H:%M (EST)')

    return jsonify({
        'success': True,
        'message': 'Authentication successful',
        'username': license_key.username,
        'expires_at': expires_formatted,
        'time_remaining': license_key.time_remaining(),
        'is_permanent': license_key.expires_at is None
    }), 200

@app.route('/api/admin/stats', methods=['GET'])
def get_stats():
    """Get system statistics (API)"""
    total_licenses = LicenseKey.query.count()
    active_licenses = LicenseKey.query.filter_by(is_active=True, is_suspended=False).count()
    suspended_licenses = LicenseKey.query.filter_by(is_suspended=True).count()

    expired_count = sum(1 for lic in LicenseKey.query.all() if lic.is_expired())

    yesterday = datetime.utcnow() - timedelta(days=1)
    recent_logins = LoginHistory.query.filter(LoginHistory.timestamp >= yesterday).count()

    return jsonify({
        'success': True,
        'stats': {
            'total_licenses': total_licenses,
            'active_licenses': active_licenses,
            'suspended_licenses': suspended_licenses,
            'expired_licenses': expired_count,
            'logins_24h': recent_logins
        }
    }), 200

# ============================================================================
# UPDATE API ENDPOINTS (for client auto-update)
# ============================================================================

@app.route('/api/version/check', methods=['GET'])
def check_version():
    """Check for available updates"""
    current_version = AppVersion.query.filter_by(is_current=True).first()

    if not current_version:
        return jsonify({
            'update_available': False,
            'message': 'No version set'
        }), 200

    return jsonify({
        'update_available': True,
        'version': current_version.version,
        'file_size': current_version.file_size,
        'release_notes': current_version.release_notes,
        'download_url': url_for('download_update', version_id=current_version.id, _external=True)
    }), 200

@app.route('/api/version/download/<int:version_id>')
def download_update(version_id):
    """Download update package"""
    version = AppVersion.query.get_or_404(version_id)

    filepath = os.path.join(app.config['UPLOAD_FOLDER'], version.filename)

    if not os.path.exists(filepath):
        return jsonify({'error': 'Update file not found'}), 404

    # Increment download count
    version.download_count += 1
    db.session.commit()

    return send_file(
        filepath,
        as_attachment=True,
        download_name=version.filename,
        mimetype='application/zip'
    )

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
    print("=" * 60)
    print("🌐 Web Admin Panel: http://localhost:5000/admin")
    print("=" * 60)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
