  inline bool SetTemperature(unsigned long iPoint, su2double temperature) final {
    Primitive(iPoint, indices.Temperature()) = temperature;
    return temperature <= 0.0;
  }
