const express = require('express');
const app = express();
const port = 3000; // Change this to your desired port number
const path = require('path');

// Serve firmware metadata file
app.get('/firmware.json', (req, res) => {
  res.sendFile(path.join(__dirname, 'firmware.json'));
});

// Serve firmware binary file
app.get('/firmware.bin', (req, res) => {
  res.sendFile(path.join(__dirname, 'index.ino.bin'));
});
app.get('/index.ino.bin', (req, res) => {
  res.sendFile(path.join(__dirname, 'index.ino.bin'));
});
app.get('/download', (req, res) => {
  res.sendFile(path.join(__dirname, 'index.ino.bin'));
});
// Start the server
app.listen(port, () => {
  console.log(`Server is running on http://localhost:${port}`);
});
