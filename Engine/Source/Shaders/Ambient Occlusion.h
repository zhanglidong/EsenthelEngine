/******************************************************************************/
BUFFER(AO)
   VecH AmbColor, AmbNSColor;
   Half AmbMaterial=1; // if apply Material Ambient
   Half AmbContrast=1.0,
        AmbScale   =2.5,
        AmbBias    =0.3;
   Vec2 AmbRange   =Vec2(0.3, 0.3);
BUFFER_END
/******************************************************************************/
