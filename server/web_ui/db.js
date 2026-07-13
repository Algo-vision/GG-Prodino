/**
 * db.js - SQLite Database Module for User Management
 */

const Database = require('better-sqlite3');
const path = require('path');
const fs = require('fs');

// Ensure data directory exists
const dataDir = path.join(__dirname, 'data');
if (!fs.existsSync(dataDir)) {
    fs.mkdirSync(dataDir, { recursive: true });
}

const dbPath = path.join(dataDir, 'grk_users.db');
const db = new Database(dbPath);

// Initialize database schema
db.exec(`
    CREATE TABLE IF NOT EXISTS users (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        email TEXT UNIQUE NOT NULL,
        name TEXT,
        role TEXT DEFAULT 'user',
        access_type TEXT DEFAULT 'all',
        created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
        last_login DATETIME,
        created_by TEXT
    );

    CREATE TABLE IF NOT EXISTS user_devices (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        user_id INTEGER NOT NULL,
        serial_number TEXT NOT NULL,
        assigned_at DATETIME DEFAULT CURRENT_TIMESTAMP,
        assigned_by TEXT,
        FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
        UNIQUE(user_id, serial_number)
    );

    CREATE TABLE IF NOT EXISTS sessions (
        id TEXT PRIMARY KEY,
        user_id INTEGER,
        expires_at DATETIME,
        FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
    );
`);

console.log('📦 Database initialized at:', dbPath);

// User Management Functions
const userQueries = {
    findByEmail: db.prepare('SELECT * FROM users WHERE email = ?'),
    findById: db.prepare('SELECT * FROM users WHERE id = ?'),
    getAll: db.prepare('SELECT id, email, name, role, access_type, created_at, last_login, created_by FROM users ORDER BY created_at DESC'),
    create: db.prepare('INSERT INTO users (email, name, role, access_type, created_by) VALUES (?, ?, ?, ?, ?)'),
    updateLastLogin: db.prepare('UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = ?'),
    updateName: db.prepare('UPDATE users SET name = ? WHERE id = ?'),
    delete: db.prepare("DELETE FROM users WHERE id = ? AND role != 'admin'"),
    countAdmins: db.prepare("SELECT COUNT(*) as count FROM users WHERE role = 'admin'"),
};

// Device Assignment Functions
const deviceQueries = {
    getByUserId: db.prepare('SELECT serial_number FROM user_devices WHERE user_id = ?'),
    assign: db.prepare('INSERT OR IGNORE INTO user_devices (user_id, serial_number, assigned_by) VALUES (?, ?, ?)'),
    remove: db.prepare('DELETE FROM user_devices WHERE user_id = ? AND serial_number = ?'),
    removeAll: db.prepare('DELETE FROM user_devices WHERE user_id = ?'),
};

// Session Functions
const sessionQueries = {
    create: db.prepare('INSERT OR REPLACE INTO sessions (id, user_id, expires_at) VALUES (?, ?, ?)'),
    find: db.prepare('SELECT * FROM sessions WHERE id = ? AND expires_at > CURRENT_TIMESTAMP'),
    delete: db.prepare('DELETE FROM sessions WHERE id = ?'),
    deleteExpired: db.prepare('DELETE FROM sessions WHERE expires_at <= CURRENT_TIMESTAMP'),
    deleteByUserId: db.prepare('DELETE FROM sessions WHERE user_id = ?'),
};

// Seed admin users if they don't exist
function seedAdmins(adminEmails) {
    const admins = adminEmails.split(',').map(e => e.trim()).filter(e => e);

    for (const email of admins) {
        const existing = userQueries.findByEmail.get(email);
        if (!existing) {
            userQueries.create.run(email, 'Admin', 'admin', 'all', 'system');
            console.log(`✅ Admin user created: ${email}`);
        } else {
            console.log(`ℹ️ Admin already exists: ${email}`);
        }
    }
}

module.exports = {
    db,

    // User operations
    findUserByEmail: (email) => userQueries.findByEmail.get(email),
    findUserById: (id) => userQueries.findById.get(id),
    getAllUsers: () => userQueries.getAll.all(),
    createUser: (email, name, role, accessType, createdBy) => {
        try {
            const result = userQueries.create.run(email, name || null, role || 'user', accessType || 'all', createdBy);
            return { success: true, id: result.lastInsertRowid };
        } catch (err) {
            if (err.code === 'SQLITE_CONSTRAINT_UNIQUE') {
                return { success: false, error: 'Email already exists' };
            }
            throw err;
        }
    },
    updateUserLastLogin: (id) => userQueries.updateLastLogin.run(id),
    updateUserName: (id, name) => userQueries.updateName.run(name, id),
    deleteUser: (id) => {
        const user = userQueries.findById.get(id);
        if (user && user.role === 'admin') {
            const adminCount = userQueries.countAdmins.get().count;
            if (adminCount <= 1) {
                return { success: false, error: 'Cannot delete the last admin' };
            }
        }
        userQueries.delete.run(id);
        return { success: true };
    },

    // Device assignment operations
    getUserDevices: (userId) => deviceQueries.getByUserId.all(userId).map(d => d.serial_number),
    assignDevice: (userId, serialNumber, assignedBy) => deviceQueries.assign.run(userId, serialNumber, assignedBy),
    removeDevice: (userId, serialNumber) => deviceQueries.remove.run(userId, serialNumber),
    removeAllDevices: (userId) => deviceQueries.removeAll.run(userId),

    // Session operations
    createSession: (sessionId, userId, expiresAt) => sessionQueries.create.run(sessionId, userId, expiresAt),
    findSession: (sessionId) => sessionQueries.find.get(sessionId),
    deleteSession: (sessionId) => sessionQueries.delete.run(sessionId),
    cleanupSessions: () => sessionQueries.deleteExpired.run(),

    // Admin seeding
    seedAdmins,
};
