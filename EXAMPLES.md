# FastCSV Examples

This document provides comprehensive examples of using FastCSV for various scenarios.

## Table of Contents

1. [Basic Reading and Writing](#basic-reading-and-writing)
2. [Dictionary Readers/Writers](#dictionary-readerswriters)
3. [Custom Dialects](#custom-dialects)
4. [Large File Processing](#large-file-processing)
5. [Format Detection](#format-detection)
6. [Error Handling](#error-handling)
7. [Performance Tips](#performance-tips)

## Basic Reading and Writing

### Reading CSV Files

```python
import fastcsv

# Read from file
with open('data.csv', 'r') as f:
    reader = fastcsv.reader(f)
    for row in reader:
        print(row)
```

### Writing CSV Files

```python
import fastcsv

# Write to file
with open('output.csv', 'w', newline='') as f:
    writer = fastcsv.writer(f)
    writer.writerow(['Name', 'Age', 'City'])
    writer.writerow(['John', '30', 'New York'])
    writer.writerow(['Jane', '25', 'Boston'])
```

### Reading from String

```python
import fastcsv
import io

data = "name,age,city\nJohn,30,New York\nJane,25,Boston"
f = io.StringIO(data)

reader = fastcsv.reader(f)
for row in reader:
    print(row)
```

## Dictionary Readers/Writers

### DictReader

```python
import fastcsv

with open('data.csv', 'r') as f:
    reader = fastcsv.DictReader(f)
    for row in reader:
        print(f"{row['name']} is {row['age']} years old")
```

### DictWriter

```python
import fastcsv

fieldnames = ['name', 'age', 'city']
with open('output.csv', 'w', newline='') as f:
    writer = fastcsv.DictWriter(f, fieldnames=fieldnames)
    writer.writeheader()
    writer.writerow({'name': 'John', 'age': '30', 'city': 'New York'})
    writer.writerow({'name': 'Jane', 'age': '25', 'city': 'Boston'})
```

### DictReader with Custom Fieldnames

```python
import fastcsv

# CSV file without header
with open('data.csv', 'r') as f:
    reader = fastcsv.DictReader(f, fieldnames=['name', 'age', 'city'])
    for row in reader:
        print(row)
```

## Custom Dialects

### Register Custom Dialect

```python
import fastcsv

# Register semicolon-delimited dialect
fastcsv.register_dialect('semicolon', delimiter=';', quotechar='"')

# Use the dialect
with open('data.csv', 'r') as f:
    reader = fastcsv.reader(f, dialect='semicolon')
    for row in reader:
        print(row)
```

### Custom Delimiter

```python
import fastcsv

# Use pipe delimiter
with open('data.csv', 'r') as f:
    reader = fastcsv.reader(f, delimiter='|')
    for row in reader:
        print(row)
```

### Custom Quote Character

```python
import fastcsv

# Use single quotes
with open('data.csv', 'r') as f:
    reader = fastcsv.reader(f, quotechar="'")
    for row in reader:
        print(row)
```

## Large File Processing

### Memory-Mapped Reader

For very large files that don't fit in memory:

```python
import fastcsv

# Use memory-mapped I/O
reader = fastcsv.mmap_reader('large_file.csv')
for row in reader:
    process(row)  # Your processing function
```

### Memory-Mapped DictReader

```python
import fastcsv

reader = fastcsv.mmap_DictReader('large_file.csv')
for row in reader:
    print(row['column_name'])
```

### Processing in Chunks

```python
import fastcsv

reader = fastcsv.mmap_reader('large_file.csv')
chunk_size = 1000
chunk = []

for row in reader:
    chunk.append(row)
    if len(chunk) >= chunk_size:
        process_chunk(chunk)  # Process chunk
        chunk = []

# Process remaining rows
if chunk:
    process_chunk(chunk)
```

## Format Detection

### Automatic Format Detection

```python
import fastcsv

with open('data.csv', 'rb') as f:
    sample = f.read(1024)
    dialect = fastcsv.Sniffer().sniff(sample.decode('utf-8', errors='ignore'))
    
    f.seek(0)
    reader = fastcsv.reader(f, dialect=dialect)
    for row in reader:
        print(row)
```

### Detect Header

```python
import fastcsv

with open('data.csv', 'rb') as f:
    sample = f.read(1024)
    sniffer = fastcsv.Sniffer()
    dialect = sniffer.sniff(sample.decode('utf-8', errors='ignore'))
    has_header = sniffer.has_header(sample.decode('utf-8', errors='ignore'))
    
    f.seek(0)
    if has_header:
        reader = fastcsv.DictReader(f, dialect=dialect)
    else:
        reader = fastcsv.reader(f, dialect=dialect)
    
    for row in reader:
        print(row)
```

## Error Handling

### Handling Parsing Errors

```python
import fastcsv

try:
    with open('data.csv', 'r') as f:
        reader = fastcsv.reader(f)
        for row in reader:
            print(row)
except fastcsv.Error as e:
    print(f"CSV parsing error: {e}")
```

### Handling File Errors

```python
import fastcsv
import os

filename = 'data.csv'
if not os.path.exists(filename):
    print(f"File {filename} not found")
else:
    try:
        reader = fastcsv.mmap_reader(filename)
        for row in reader:
            print(row)
    except Exception as e:
        print(f"Error reading file: {e}")
```

## Performance Tips

### Use Memory-Mapped Reader for Large Files

```python
# Good for large files
reader = fastcsv.mmap_reader('large_file.csv')

# Less efficient for large files
with open('large_file.csv', 'r') as f:
    reader = fastcsv.reader(f)
```

### Batch Processing

```python
# Process in batches
reader = fastcsv.mmap_reader('large_file.csv')
batch = []
batch_size = 10000

for row in reader:
    batch.append(row)
    if len(batch) >= batch_size:
        process_batch(batch)
        batch = []
```

### Avoid Unnecessary Conversions

```python
# Good: Direct iteration
reader = fastcsv.reader(file)
for row in reader:
    process(row)

# Less efficient: Converting to list first
rows = list(fastcsv.reader(file))
for row in rows:
    process(row)
```

## Advanced Examples

### Filtering Rows

```python
import fastcsv

reader = fastcsv.mmap_reader('data.csv')
filtered = [row for row in reader if int(row[1]) > 25]  # Age > 25
```

### Aggregating Data

```python
import fastcsv
from collections import defaultdict

reader = fastcsv.mmap_DictReader('sales.csv')
totals = defaultdict(float)

for row in reader:
    totals[row['category']] += float(row['amount'])

for category, total in totals.items():
    print(f"{category}: {total}")
```

### Converting to Pandas DataFrame

```python
import fastcsv
import pandas as pd

reader = fastcsv.mmap_DictReader('data.csv')
df = pd.DataFrame(list(reader))
print(df)
```

### Processing Multiple Files

```python
import fastcsv
import glob

for filename in glob.glob('data/*.csv'):
    reader = fastcsv.mmap_reader(filename)
    for row in reader:
        process_row(row, filename)
```

## Real-World Use Cases

### Log File Analysis

```python
import fastcsv

reader = fastcsv.mmap_reader('access.log')
error_count = 0

for row in reader:
    if row[2] == 'ERROR':  # Assuming error status in column 2
        error_count += 1

print(f"Total errors: {error_count}")
```

### Data Migration

```python
import fastcsv

# Read from source
reader = fastcsv.mmap_DictReader('source.csv')

# Write to destination
with open('destination.csv', 'w', newline='') as f:
    writer = fastcsv.DictWriter(f, fieldnames=reader.fieldnames)
    writer.writeheader()
    
    for row in reader:
        # Transform data if needed
        transformed = transform_row(row)
        writer.writerow(transformed)
```

### Data Validation

```python
import fastcsv

reader = fastcsv.mmap_DictReader('data.csv')
errors = []

for i, row in enumerate(reader, start=2):  # Start at 2 (header is row 1)
    if not row['email'].endswith('@example.com'):
        errors.append(f"Row {i}: Invalid email {row['email']}")

if errors:
    print("Validation errors found:")
    for error in errors:
        print(error)
```


