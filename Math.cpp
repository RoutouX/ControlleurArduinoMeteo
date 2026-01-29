float mediane(float arr[], int size) {
  float temp[size];

  for (int i = 0; i < size; i++) {
    temp[i] = arr[i];
  }

  for (int i = 0; i < size - 1; i++) {
    for (int j = i + 1; j < size; j++) {
      if (temp[i] > temp[j]) {
        float t = temp[i];
        temp[i] = temp[j];
        temp[j] = t;
      }
    }
  }

  if (size % 2 == 1) {
    return temp[size / 2];
  }
  return (temp[size / 2 - 1] + temp[size / 2]) / 2.0f;
}
