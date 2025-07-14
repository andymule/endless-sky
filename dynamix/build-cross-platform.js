#!/usr/bin/env node

const { spawn, execSync } = require('child_process');
const path = require('path');
const fs = require('fs');

// Detect platform
const platform = process.platform;
const isWindows = platform === 'win32';

console.log(`Building on ${platform}...`);

try {
    if (isWindows) {
        // Windows: Use the batch script
        console.log('Using Windows build script...');
        const result = execSync('build-windows-fast.bat', {
            stdio: 'inherit',
            cwd: __dirname
        });
    } else {
        // macOS/Linux: Use the shell script
        console.log('Using Unix build script...');
        const result = execSync('./build-cross-platform.sh', {
            stdio: 'inherit',
            cwd: __dirname
        });
    }
} catch (error) {
    console.error('Build failed:', error.message);
    process.exit(1);
} 