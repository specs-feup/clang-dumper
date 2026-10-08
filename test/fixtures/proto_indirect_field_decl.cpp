struct Record {
  union {
    int value;
  };
};

int readValue(Record record) { return record.value; }
