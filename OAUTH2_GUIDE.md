# OAuth 2.0 Implementation Guide

This project now includes a complete **OAuth 2.0 Authorization Server** with Client Credentials flow support.

## 🔐 What is OAuth 2.0?

OAuth 2.0 is an industry-standard protocol for authorization. Instead of using static API keys, OAuth 2.0 uses **time-limited access tokens** that expire and can be refreshed, providing better security.

## 🆚 OAuth 2.0 vs API Keys

| Feature | API Keys (Old) | OAuth 2.0 (New) |
|---------|----------------|-----------------|
| Security | Static, never expire | Time-limited tokens |
| Token Refresh | No | Yes (refresh tokens) |
| Revocation | Manual only | Automatic + manual |
| Standard | Proprietary | Industry standard (RFC 6749) |
| Expiration | None | 1 hour (configurable) |
| Best For | Simple APIs | Production systems |

## 🚀 Quick Start

### 1. Start the OAuth 2.0 Server

```bash
cd backend
python oauth_app.py
```

The server will start on `http://localhost:5000`

### 2. Create an OAuth 2.0 Client

```bash
python oauth_manage.py create "My Desktop App"
```

**Save the output!** You'll get:
- `client_id` - Your application identifier
- `client_secret` - Your application password (store securely!)

Example output:
```
✓ OAuth 2.0 Client created successfully!

Client Name: My Desktop App
Client ID: abc123xyz789...
Client Secret: def456uvw012...
Grant Types: client_credentials refresh_token
Scopes: read write

Save the client_secret securely - it cannot be retrieved again
```

### 3. Configure Your C++ Client

Edit `client/config.ini`:
```ini
[OAuth]
client_id=YOUR_CLIENT_ID_HERE
client_secret=YOUR_CLIENT_SECRET_HERE
server_host=localhost
server_port=5000
```

### 4. Get an Access Token

#### Using Command Line:
```bash
python oauth_manage.py token <client_id> <client_secret>
```

#### Using curl:
```bash
curl -X POST http://localhost:5000/oauth/token \
  -d 'grant_type=client_credentials' \
  -d 'client_id=YOUR_CLIENT_ID' \
  -d 'client_secret=YOUR_CLIENT_SECRET'
```

Response:
```json
{
  "access_token": "long_random_string...",
  "token_type": "Bearer",
  "expires_in": 3600,
  "refresh_token": "another_long_string...",
  "scope": "read write"
}
```

### 5. Use the Access Token

```bash
curl -H 'Authorization: Bearer YOUR_ACCESS_TOKEN' \
  http://localhost:5000/api/protected
```

## 📚 API Endpoints

### Token Endpoint
**POST** `/oauth/token`

Get access tokens using client credentials or refresh tokens.

**Client Credentials Grant:**
```bash
POST /oauth/token
Content-Type: application/x-www-form-urlencoded

grant_type=client_credentials&
client_id=YOUR_CLIENT_ID&
client_secret=YOUR_CLIENT_SECRET&
scope=read write
```

**Refresh Token Grant:**
```bash
POST /oauth/token
Content-Type: application/x-www-form-urlencoded

grant_type=refresh_token&
refresh_token=YOUR_REFRESH_TOKEN&
client_id=YOUR_CLIENT_ID&
client_secret=YOUR_CLIENT_SECRET
```

### Token Introspection
**POST** `/oauth/introspect`

Check if a token is valid and get its metadata.

```bash
POST /oauth/introspect
Content-Type: application/x-www-form-urlencoded

token=YOUR_ACCESS_TOKEN
```

### Token Revocation
**POST** `/oauth/revoke`

Revoke an access token or refresh token.

```bash
POST /oauth/revoke
Content-Type: application/x-www-form-urlencoded

token=YOUR_TOKEN&
token_type_hint=access_token
```

### Protected Resource
**GET** `/api/protected`

Example of an OAuth 2.0 protected resource.

```bash
GET /api/protected
Authorization: Bearer YOUR_ACCESS_TOKEN
```

### Admin Endpoints

**Create Client:**
```bash
POST /admin/clients
Content-Type: application/json

{
  "client_name": "My Application",
  "grant_types": "client_credentials refresh_token",
  "scopes": "read write"
}
```

**List Clients:**
```bash
GET /admin/clients
```

**Deactivate Client:**
```bash
DELETE /admin/clients/{id}
```

## 🔧 Configuration

Edit `oauth_app.py` to customize:

```python
# Token expiration times
ACCESS_TOKEN_EXPIRY = 3600  # 1 hour (in seconds)
REFRESH_TOKEN_EXPIRY = 2592000  # 30 days (in seconds)
```

## 🛡️ Security Features

### 1. **Time-Limited Tokens**
- Access tokens expire after 1 hour
- Reduces risk if token is compromised
- Forces clients to refresh periodically

### 2. **Refresh Tokens**
- Long-lived tokens (30 days)
- Used to get new access tokens
- Single-use (revoked after refresh)

### 3. **Token Hashing**
- Tokens are hashed with SHA-256 before storage
- Database compromise doesn't expose active tokens
- Same security as password hashing

### 4. **Token Revocation**
- Tokens can be manually revoked
- Expired tokens are automatically invalid
- Supports immediate access termination

### 5. **Client Authentication**
- Client secrets are hashed (SHA-256)
- Secrets never transmitted in plaintext (HTTPS recommended)
- Failed authentication attempts can be logged

## 🔄 Token Refresh Flow

When your access token expires:

1. Client detects 401 Unauthorized response
2. Client uses refresh_token to get new tokens
3. Server validates refresh token
4. Server issues new access_token and refresh_token
5. Old refresh_token is revoked
6. Client retries original request with new token

Example:
```python
# Get new tokens using refresh token
response = requests.post('http://localhost:5000/oauth/token', data={
    'grant_type': 'refresh_token',
    'refresh_token': stored_refresh_token,
    'client_id': client_id,
    'client_secret': client_secret
})

new_tokens = response.json()
# Store new access_token and refresh_token
```

## 🧪 Testing

### Test Complete Flow:

```bash
# 1. Create client
python oauth_manage.py create "Test App"

# 2. Save the client_id and client_secret

# 3. Get access token
python oauth_manage.py token <client_id> <client_secret>

# 4. Test the access token
python oauth_manage.py test <access_token>

# 5. Introspect the token
python oauth_manage.py introspect <access_token>

# 6. Revoke the token
python oauth_manage.py revoke <access_token>

# 7. Try to use revoked token (should fail)
python oauth_manage.py test <access_token>
```

## 📊 Database Schema

### OAuth 2.0 Tables:

**oauth_client** - OAuth 2.0 clients
- `client_id` - Unique identifier
- `client_secret_hash` - Hashed secret
- `client_name` - Human-readable name
- `grant_types` - Allowed OAuth flows
- `scopes` - Permitted scopes
- `is_active` - Active status

**access_token** - Access tokens
- `token_hash` - Hashed token
- `client_id` - Associated client
- `expires_at` - Expiration time
- `scope` - Granted permissions
- `revoked` - Revocation status

**refresh_token** - Refresh tokens
- `token_hash` - Hashed token
- `client_id` - Associated client
- `expires_at` - Expiration time
- `scope` - Granted permissions
- `revoked` - Revocation status

## 🚀 Production Deployment

### Required Security Measures:

1. **Use HTTPS** - Never use OAuth 2.0 over HTTP in production
2. **Secure Storage** - Store client secrets in environment variables
3. **Rate Limiting** - Prevent brute force attacks
4. **Logging** - Monitor failed authentication attempts
5. **Token Rotation** - Short-lived access tokens (15-60 min)
6. **Database Encryption** - Encrypt database at rest

### Environment Variables:

```bash
export SECRET_KEY="your-secret-key-here"
export DATABASE_URL="postgresql://user:pass@localhost/oauth"
export TOKEN_EXPIRY=900  # 15 minutes
```

### Production Server:

```bash
pip install gunicorn
gunicorn -w 4 -b 0.0.0.0:5000 oauth_app:app
```

## 🔗 Standards Compliance

This implementation follows these RFCs:

- **RFC 6749** - OAuth 2.0 Authorization Framework
- **RFC 6750** - Bearer Token Usage
- **RFC 7009** - Token Revocation
- **RFC 7662** - Token Introspection

## ❓ FAQ

**Q: What's the difference from the old system?**
A: The old system used permanent API keys. OAuth 2.0 uses temporary access tokens that expire and can be refreshed.

**Q: Can I still use the old API key system?**
A: Yes, both systems coexist. Run `app.py` for API keys or `oauth_app.py` for OAuth 2.0.

**Q: Which should I use?**
A: Use OAuth 2.0 for production systems. It's more secure and industry-standard.

**Q: How do I migrate from API keys to OAuth 2.0?**
A: Create an OAuth client, update your application to use access tokens, then deprecate the API key endpoints.

**Q: What happens when a token expires?**
A: Your application receives a 401 Unauthorized response. Use the refresh token to get a new access token.

**Q: Can I extend token expiration?**
A: Yes, edit `ACCESS_TOKEN_EXPIRY` and `REFRESH_TOKEN_EXPIRY` in `oauth_app.py`.

**Q: How do I revoke a user's access immediately?**
A: Use `python oauth_manage.py revoke <token>` or the `/oauth/revoke` endpoint.

## 📖 Additional Resources

- [OAuth 2.0 Simplified](https://www.oauth.com/)
- [RFC 6749 - OAuth 2.0](https://tools.ietf.org/html/rfc6749)
- [OAuth 2.0 Playground](https://www.oauth.com/playground/)

## 🐛 Troubleshooting

**Problem:** `invalid_client` error
- **Solution:** Verify client_id and client_secret are correct

**Problem:** `invalid_token` error
- **Solution:** Token may be expired or revoked. Get a new token.

**Problem:** `unsupported_grant_type` error
- **Solution:** Check that the client supports the requested grant type

**Problem:** Can't refresh token
- **Solution:** Refresh tokens are single-use. After refreshing, use the NEW refresh token.

---

**Ready to use OAuth 2.0? Start with the Quick Start guide above!**
