const express = require('express');
const path = require('path');
require('dotenv').config();

const app = express();
const PORT = process.env.UI_PORT || 5556;

app.use(express.static(path.join(__dirname, 'public')));

app.use((req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.listen(PORT, () => {
    console.log(`UI Server running on http://localhost:${PORT}`);
});
