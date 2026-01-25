/**
 * auth.js - Authentication Module using Passport.js with Google OAuth
 */

const passport = require('passport');
const GoogleStrategy = require('passport-google-oauth20').Strategy;
const db = require('./db');

// Configure Passport serialization
passport.serializeUser((user, done) => {
    done(null, user.id);
});

passport.deserializeUser((id, done) => {
    const user = db.findUserById(id);
    done(null, user);
});

// Configure Google OAuth Strategy
function initializePassport() {
    const clientID = process.env.GOOGLE_CLIENT_ID;
    const clientSecret = process.env.GOOGLE_CLIENT_SECRET;
    const callbackURL = process.env.GOOGLE_CALLBACK_URL || '/auth/google/callback';

    if (!clientID || !clientSecret) {
        console.error('❌ GOOGLE_CLIENT_ID and GOOGLE_CLIENT_SECRET must be set in .env');
        return false;
    }

    passport.use(new GoogleStrategy({
        clientID,
        clientSecret,
        callbackURL,
        scope: ['profile', 'email']
    }, (accessToken, refreshToken, profile, done) => {
        const email = profile.emails[0].value.toLowerCase();
        const name = profile.displayName;

        // Check if user is whitelisted
        let user = db.findUserByEmail(email);

        if (!user) {
            // User not in whitelist - deny access
            return done(null, false, { message: 'Access denied. Contact admin for access.' });
        }

        // Update user name and last login
        db.updateUserName(user.id, name);
        db.updateUserLastLogin(user.id);

        // Refresh user data after update
        user = db.findUserById(user.id);

        return done(null, user);
    }));

    console.log('✅ Passport Google OAuth configured');
    return true;
}

// Middleware to check if user is authenticated
function isAuthenticated(req, res, next) {
    if (req.isAuthenticated()) {
        return next();
    }

    // For API requests, return 401
    if (req.xhr || req.path.startsWith('/api/')) {
        return res.status(401).json({ error: 'Not authenticated' });
    }

    // For page requests, redirect to login
    res.redirect('/login.html');
}

// Middleware to check if user is admin
function isAdmin(req, res, next) {
    if (req.isAuthenticated() && req.user && req.user.role === 'admin') {
        return next();
    }

    if (req.xhr || req.path.startsWith('/api/')) {
        return res.status(403).json({ error: 'Admin access required' });
    }

    res.redirect('/');
}

// Get devices user can access
function getUserAllowedDevices(user) {
    if (!user) return [];

    // Admins can see all devices
    if (user.role === 'admin') return null; // null means all devices

    // Users with 'all' access see everything
    if (user.access_type === 'all') return null;

    // Restricted users only see assigned devices
    return db.getUserDevices(user.id);
}

// Filter device list based on user permissions
function filterDevicesForUser(devices, user) {
    const allowed = getUserAllowedDevices(user);

    // null means all devices allowed
    if (allowed === null) return devices;

    // Filter to only allowed serial numbers
    return devices.filter(d => allowed.includes(d.serialNumber));
}

module.exports = {
    passport,
    initializePassport,
    isAuthenticated,
    isAdmin,
    getUserAllowedDevices,
    filterDevicesForUser,
};
