  bool SetSoundSpeed(unsigned long iPoint, su2double soundspeed2) final {
    if (soundspeed2 < 0.0) return true;
    Primitive(iPoint,nDim+4) = sqrt(soundspeed2);
    return false;
  }
