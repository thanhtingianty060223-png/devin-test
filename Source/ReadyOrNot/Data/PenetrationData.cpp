// Copyright Void Interactive, 2022

#include "PenetrationData.h"
#include "ReadyOrNot.h"

FMaterialPenetration UPenetrationData::GetPenetrationData(EPhysicalSurface Surface)
{
	switch (Surface)
	{
	case SurfaceType1:
		return RON_Aluminium;
	case SurfaceType2:
		return RON_Asphalt;
	case SurfaceType3:
		return RON_Brick;
	case SurfaceType4:
		return RON_CarbonFibre;
	case SurfaceType5:
		return RON_Cardboard;
	case SurfaceType6:
		return RON_Ceramic;
	case SurfaceType7:
		return RON_ConcreteSoft;
	case SurfaceType8:
		return RON_ConcreteStrong;
	case SurfaceType9:
		return RON_Dirt;
	case SurfaceType10:
		return RON_Drywall;
	case SurfaceType11:
		return RON_Electrical;
	case SurfaceType12:
		return RON_EnergyShield;
	case SurfaceType13:
		return RON_Fabric_Carpet;
	case SurfaceType14:
		return RON_Fabric_Stuffing;
	case SurfaceType15:
		return RON_Fabric_Thin;
	case SurfaceType16:
		return RON_Flesh;
	case SurfaceType17:
		return RON_Galvanized;
	case SurfaceType18:
		return RON_Glass_Plate;
	case SurfaceType19:
		return RON_Glass_Windshield;
	case SurfaceType20:
		return RON_Grass;
	case SurfaceType21:
		return RON_Gravel;
	case SurfaceType22:
		return RON_Ice;
	case SurfaceType23:
		return RON_Lava;
	case SurfaceType24:
		return RON_Lead;
	case SurfaceType25:
		return RON_Leaves;
	case SurfaceType26:
		return RON_Limestone;
	case SurfaceType27:
		return RON_Mahogany;
	case SurfaceType28:
		return RON_Marble_Coated;
	case SurfaceType29:
		return RON_Marble_Thick;
	case SurfaceType30:
		return RON_Mud;
	case SurfaceType31:
		return RON_Oil;
	case SurfaceType32:
		return RON_Paper;
	case SurfaceType33:
		return RON_Pine;
	case SurfaceType34:
		return RON_Plaster;
	case SurfaceType35:
		return RON_Plastic;
	case SurfaceType36:
		return RON_Plywood;
	case SurfaceType37:
		return RON_Polystyrene;
	case SurfaceType38:
		return RON_Powder;
	case SurfaceType39:
		return RON_Rock;
	case SurfaceType40:
		return RON_Rubber;
	case SurfaceType41:
		return RON_Sand;
	case SurfaceType42:
		return RON_Snow;
	case SurfaceType43:
		return RON_Soil;
	case SurfaceType44:
		return RON_Steel;
	case SurfaceType45:
		return RON_Tin;
	case SurfaceType46:
		return RON_Treewood;
	case SurfaceType47:
		return RON_Wallpaper;
	case SurfaceType48:
		return RON_Water;

	// Addons
	case SurfaceType49:
		return DefaultPenetrationData; // RON_Shield
	case SurfaceType50:
		return DefaultPenetrationData; // RON_Cloth
	case SurfaceType51:
		return RON_Vehicle;
	case SurfaceType52:
		return RON_Bulletproof_Glass;
	}
	return DefaultPenetrationData;
}
