unsigned int mediane(float arr[], int size) {
  unsigned int temp[size];

  for (int i = 0; i < size; i++) {
    temp[i] = arr[i];
  }

  for (int i = 0; i < size - 1; i++) {
    for (int j = i + 1; j < size; j++) {
      if (temp[i] > temp[j]) {
        unsigned int t = temp[i];
        temp[i] = temp[j];
        temp[j] = t;
      }
    }
  }

  return (temp[size / 2 - 1] + temp[size / 2]) / 2;
}