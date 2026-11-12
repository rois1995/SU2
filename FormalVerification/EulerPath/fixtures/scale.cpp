su2double Vol = geometry->nodes->GetVolume(iPoint) + geometry->nodes->GetPeriodicVolume(iPoint);
        su2double Delta = nodes->GetDelta_Time(iPoint) / Vol;
