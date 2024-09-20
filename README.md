# Distributed File System (COMP-8567 Project)

This project implements a distributed file system using socket programming. It involves three servers (`Smain`, `Spdf`, and `Stext`) that work together to handle client file upload, download, and deletion requests based on file types (`.c`, `.pdf`, `.txt`). Clients interact solely with the `Smain` server, which transparently manages background file transfers to the other servers.

## Table of Contents
- [Overview](#overview)
- [Project Structure](#project-structure)
- [Features](#features)
- [Commands](#commands)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [License](#license)

## Overview

The distributed file system is designed to simulate file storage and retrieval for different file types:
- `.c` files are stored locally in `Smain`.
- `.pdf` files are forwarded to `Spdf`.
- `.txt` files are forwarded to `Stext`.

Clients are unaware of this distribution and assume all files are stored in `Smain`. The servers communicate using sockets, and Smain handles multiple client requests using forking to create separate processes for each client.

## Project Structure

- **Smain.c**: The main server responsible for handling client requests and managing file storage.
- **Spdf.c**: A server dedicated to handling `.pdf` files, forwarding and receiving files from `Smain`.
- **Stext.c**: A server responsible for handling `.txt` files, communicating with `Smain`.
- **client24s.c**: The client application that communicates with `Smain` to upload, download, and delete files.

## Features

- **Multi-client Support**: Handles multiple clients simultaneously using forking to create individual processes for each client request.
- **File Upload and Download**: Clients can upload and download `.c`, `.pdf`, and `.txt` files.
- **Background File Transfers**: Smain forwards `.pdf` and `.txt` files to Spdf and Stext, respectively, while keeping `.c` files locally.
- **File Deletion**: Clients can request the deletion of files, and Smain will manage the removal from the appropriate server.
- **Tar File Creation**: Clients can request a tar archive of all files of a specific type, which is compiled from Smain, Spdf, and Stext.

## Commands

The client application supports the following commands:

- `ufile <filename> <destination_path>`: Uploads a file to `Smain`.
  - Example: `ufile sample.c ~smain/folder1/folder2`
- `dfile <filename>`: Downloads a file from `Smain`.
  - Example: `dfile ~smain/folder1/sample.txt`
- `rmfile <filename>`: Removes a file from `Smain`.
  - Example: `rmfile ~smain/folder1/sample.pdf`
- `dtar <filetype>`: Downloads a tar archive of all files of the specified type.
  - Example: `dtar .pdf`
- `display <pathname>`: Displays a list of `.c`, `.pdf`, and `.txt` files in the specified path.
  - Example: `display ~smain/folder1`

## Requirements

- **GCC (GNU Compiler Collection)**: To compile the C programs.
- **Linux Environment**: For socket programming and file handling.
- **Multiple Machines/Terminals**: For testing communication between servers and clients.
- **Basic knowledge of C programming**: To understand and modify the code if needed.

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/your-username/distributed-file-system.git
