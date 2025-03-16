# Database Indexing using Heap Files & Hash Tables

## Overview
This project was developed for the **Database Systems Implementation** course at the National and Kapodistrian University of Athens. It focuses on understanding the internal workings of database systems, specifically block-level storage management, record-level management, and the role of indexing in improving performance.

The project includes the implementation of:
- **Heap File (HP) System**
- **Static Hash Table (HT) System**
- **Secondary Hash Table (SHT) System**

All operations are built on top of the provided **Block File library (`bf.h`)**, which simulates disk block accesses, mimicking how database systems manage data storage on disk.

---

## Components

### 1. Heap File (HP)
Implements a heap-based file organization where records are stored in no particular order. Supports:
- File creation and opening
- Record insertion, deletion, and searching
- File closing

---

### 2. Static Hash Table (HT)
Implements static hashing to enable efficient access to records based on their key. Includes:
- Index file creation
- Record insertion, deletion, and retrieval
- File closing

---

### 3. Secondary Hash Table (SHT)
Provides secondary indexing on the `surname` attribute, allowing:
- Creation of secondary index files
- Insertion and searching of records using secondary keys
- Linking to primary hash tables

---

### 4. Hash Statistics
Includes a utility function to analyze hash table performance by reporting:
- Number of blocks in the file
- Record distribution per bucket
- Overflow block usage

---

## Data Structures

Records have the following structure:
```c
typedef struct {
    int id;
    char name[15];
    char surname[25];
    char address[50];
} Record;
```
Additional metadata structures (`HP_info`, `HT_info`, `SHT_info`) maintain information needed to manage each file type.

---

## Dependencies
- **C Language**
- **Block File Library (`bf.h`)** for simulating low-level block-based disk operations
- Unix/Linux environment

---

## Compilation
Compile using:
```bash
make
```

---

## Usage
Run the files created in build.

---

## Authors
• [Panagiotis Chatzimichos](https://github.com/pchatz000)\
• [Danae Karageorgopoulou ](https://github.com/danaikar)
