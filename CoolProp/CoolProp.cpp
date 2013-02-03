#if defined(_MSC_VER)
#define _CRTDBG_MAP_ALLOC
#define _CRT_SECURE_NO_WARNINGS
#include <crtdbg.h>
#endif

#include "CoolProp.h"

#if defined(__ISWINDOWS__)
#include <windows.h>
#include "REFPROP.h"
#endif

#include <iostream>
#include <stdlib.h>
#include <vector>
#include <exception>
#include <stdio.h>
#include "string.h"
#include "FluidClass.h"
#include "CoolPropTools.h"
#include "CPExceptions.h"
#include "Brine.h"
#include "Solvers.h"
#include "CPState.h"
#include "IncompLiquid.h"

#ifndef DBL_EPSILON
	#include <limits>
	#define DBL_EPSILON std::numeric_limits<double>::epsilon()
#endif

// Function prototypes
double rho_TP(double T, double p);
double _Props(std::string Output,std::string Name1, double Prop1, std::string Name2, double Prop2, std::string Ref);
double _CoolProp_Fluid_Props(long iOutput, long iName1, double Value1, long iName2, double Value2, Fluid *pFluid, bool SinglePhase = false);

static std::string err_string;
int debug_level=0;

Fluid * pFluid;

// This is very hacky, but pull the subversion revision from the file
#include "svnrevision.h" // Contents are like "long svnrevision = 286;"
#include "version.h" // Contents are like "char version [] ="2.5";"

int global_Phase = -1;
bool global_SinglePhase = false;
bool global_SaturatedL = false;
bool global_SaturatedV = false;

// This is a map of all possible strings to a unique identifier
std::pair<std::string, long> map_data[] = {
	std::make_pair(std::string("E"),iPcrit),
	std::make_pair(std::string("M"),iMM),
	std::make_pair(std::string("w"),iAccentric),
	std::make_pair(std::string("R"),iTtriple),
	std::make_pair(std::string("N"),iRhocrit),
	std::make_pair(std::string("B"),iTcrit),

	std::make_pair(std::string("pcrit"),iPcrit),
	std::make_pair(std::string("molemass"),iMM),
	std::make_pair(std::string("accentric"),iAccentric),
	std::make_pair(std::string("dipole"),iDipole),
	std::make_pair(std::string("Tmin"),iTmin),
	std::make_pair(std::string("t"),iTmin),
	std::make_pair(std::string("Ttriple"),iTtriple),
	std::make_pair(std::string("rhocrit"),iRhocrit),
	std::make_pair(std::string("Tcrit"),iTcrit),

	std::make_pair(std::string("Q"),iQ),
	std::make_pair(std::string("T"),iT),
    std::make_pair(std::string("P"),iP),
	std::make_pair(std::string("D"),iD),
	std::make_pair(std::string("C"),iC),
	std::make_pair(std::string("C0"),iC0),
	std::make_pair(std::string("O"),iO),
	std::make_pair(std::string("U"),iU),
	std::make_pair(std::string("H"),iH),
	std::make_pair(std::string("S"),iS),
	std::make_pair(std::string("A"),iA),
	std::make_pair(std::string("G"),iG),
	std::make_pair(std::string("V"),iV),
	std::make_pair(std::string("L"),iL),
	std::make_pair(std::string("I"),iI),
	std::make_pair(std::string("SurfaceTension"),iI),
	std::make_pair(std::string("dpdT"),iDpdT),
	std::make_pair(std::string("drhodT|p"),iDrhodT_p),
	std::make_pair(std::string("M"),iMM),
};
//Now actually construct the map
std::map<std::string, long> param_map(map_data,
    map_data + sizeof map_data / sizeof map_data[0]);

// This is a map of all unique identifiers to std::strings with the default units
// that are used internally
std::pair<long, std::string> units_data[] = {
	std::make_pair(iPcrit, std::string("kPa")),
	std::make_pair(iMM, std::string("kg/kmol")),
	std::make_pair(iAccentric, std::string("-")),
	std::make_pair(iTtriple, std::string("K")),
	std::make_pair(iRhocrit, std::string("kg/m^3")),
	std::make_pair(iTcrit, std::string("K")),

	std::make_pair(iQ, std::string("")),
	std::make_pair(iT, std::string("K")),
    std::make_pair(iP, std::string("kPa")),
	std::make_pair(iD, std::string("kg/m^3")),
	std::make_pair(iC, std::string("kJ/kg/K")),
	std::make_pair(iC0, std::string("kJ/kg/K")),
	std::make_pair(iO, std::string("kJ/kg/K")),
	std::make_pair(iU, std::string("kJ/kg")),
	std::make_pair(iH, std::string("kJ/kg")),
	std::make_pair(iS, std::string("kJ/kg/K")),
	std::make_pair(iA, std::string("m/s")),
	std::make_pair(iG, std::string("kJ/kg")),
	std::make_pair(iV, std::string("Pa*s")),
	std::make_pair(iL, std::string("kW/m/K")),
	std::make_pair(iI, std::string("N/m")),
	std::make_pair(iDpdT, std::string("kPa/K")),
	std::make_pair(iDrhodT_p, std::string("kg/K/m^3"))
};

//Now actually construct the map
std::map<long, std::string> units_map(units_data,
		units_data + sizeof units_data / sizeof units_data[0]);

FluidsContainer Fluids = FluidsContainer();

EXPORT_CODE long CONVENTION get_svnrevision(){return svnrevision;}
EXPORT_CODE long CONVENTION get_version(char * pversion){
	strcpy(pversion,version);
	return 0;
}
std::string get_version(){return std::string(version);}

void set_debug(int level){debug_level = level;}
EXPORT_CODE int CONVENTION get_debug(){return debug_level;}
int  debug(){return debug_level;}
EXPORT_CODE void CONVENTION debug(int level){debug_level=level;}

std::string get_errstring(void){
    std::string temp = err_string;
    err_string = std::string("");
    return temp;
    }
EXPORT_CODE void CONVENTION get_errstring(char* str){
    str=(char*) get_errstring().c_str();
	err_string.clear();
    };

EXPORT_CODE char * CONVENTION get_errstringc(void){
    std::string temp = err_string;
	err_string.clear();
    return (char*)temp.c_str();
    }

EXPORT_CODE long CONVENTION get_errstring_copy(char* str){
    strcpy(str, get_errstring().c_str());
	err_string.clear();
	return strlen(str);
    };

// A function to enforce the state if known
EXPORT_CODE void CONVENTION set_phase(char *Phase_str){
	set_phase(std::string(Phase_str));
}

void set_phase(std::string Phase_str){
	if (!Phase_str.compare("Two-Phase")){
		global_SinglePhase = false;
		global_SaturatedL = false;
		global_SaturatedV = false;
		global_Phase = iTwoPhase;
	}
	else if (!Phase_str.compare("Liquid")){
		global_SinglePhase = true;
		global_SaturatedL = false;
		global_SaturatedV = false;
		global_Phase = iLiquid;
	}
	else if (!Phase_str.compare("Gas")){
		global_SinglePhase = true;
		global_SaturatedL = false;
		global_SaturatedV = false;
		global_Phase = iGas;
	}
	else if (!Phase_str.compare("Supercritical")){
		global_SinglePhase = true;
		global_SaturatedL = false;
		global_SaturatedV = false;
		global_Phase = iSupercritical;
	}
	else if (!Phase_str.compare("SaturatedL")){
		global_SinglePhase = false;
		global_SaturatedL = true;
		global_SaturatedV = false;
		global_Phase = iTwoPhase;
	}
	else if (!Phase_str.compare("SaturatedV")){
		global_SinglePhase = false;
		global_SaturatedL = false;
		global_SaturatedV = true;
		global_Phase = iTwoPhase;
	}
}

// Returns a pointer to the fluid class
Fluid* get_fluid(long iFluid){
	return Fluids.get_fluid(iFluid);
}
long get_Fluid_index(std::string FluidName)
{
	// Try to get the fluid from Fluids by name
	pFluid = Fluids.get_fluid(FluidName);
	// If NULL, didn't find it (or its alias)
	if (pFluid!=NULL)
	{
		// Find the fluid index
		return Fluids.get_fluid_index(pFluid);
	}
	else
		return -1;
}
EXPORT_CODE long CONVENTION get_Fluid_index(char * param)
{
	return get_Fluid_index(std::string(param));
}

std::string get_index_units(long index)
{
	std::map<long, std::string>::iterator it;
	// Try to find using the map
	it = units_map.find(index);
	// If it is found the iterator will not be equal to end
	if (it != units_map.end() )
	{
		// Return the index of the parameter
		return (*it).second;
	}
	else
	{
		std::cout << "Didn't match parameter: " << index << std::endl;
		return std::string("Didn't match parameter");
	}
}
EXPORT_CODE void CONVENTION get_index_units(long param, char * units)
{
	strcpy(units, (char*)get_index_units(param).c_str());
	return;
}

long get_param_index(std::string param)
{
	std::map<std::string,long>::iterator it;
	// Try to find using the map
	it = param_map.find(param);
	// If it is found the iterator will not be equal to end
	if (it != param_map.end() )
	{
		// Return the index of the parameter
		return (*it).second;
	}
	else
	{
		std::cout << "Didn't match parameter: " << param << std::endl;
		return -1;
	}
}
EXPORT_CODE long CONVENTION get_param_index(char * param)
{
	return get_param_index(std::string(param));
}
static int IsCoolPropFluid(std::string FluidName)
{
	// Try to get the fluid from Fluids by name
	try
	{
		pFluid = Fluids.get_fluid(FluidName);
	}
	catch (NotImplementedError)
	{
		return false;
	}
	// If NULL, didn't find it (or its alias)
	if (pFluid!=NULL)
	{
		return true;
	}
	else
		return false;
}

static int IsCoolPropFluid(char* Fluid)
{
	return IsCoolPropFluid(std::string(Fluid));
}

static int IsBrine(char* Ref)
{
	// First check whether it is one of the Brines that does
	// not have a pure-fluid equivalent in CoolProp
    if (
        strcmp(Ref,"HC-10")==0 || 
        strncmp(Ref,"PG",2)==0 || 
		strncmp(Ref,"EG",2)==0 || 
		strncmp(Ref,"EA",2)==0 ||
		strncmp(Ref,"MA",2)==0 || 
		strncmp(Ref,"Glycerol",8)==0 ||
		strncmp(Ref,"K2CO3",5)==0 || 
		strncmp(Ref,"CaCl2",5)==0 || 
		strncmp(Ref,"MgCl2",5)==0 || 
		strncmp(Ref,"NaCl",4)==0 || 
		strncmp(Ref,"KAC",3)==0 || 
		strncmp(Ref,"KFO",3)==0 || 
		strncmp(Ref,"LiCl",3)==0 || 
        strncmp(Ref,"NH3/H2O",7)==0
       )
    {
        return 1;
    }
	// Then check for diluants that are also pure fluids in CoolProp
	else if ( (strncmp(Ref,"Methanol",8)==0 && Ref[8] == '-') ||
		      (strncmp(Ref,"Ethanol",7)==0 && Ref[7] == '-') ||
			  (strncmp(Ref,"NH3",3)==0 && Ref[3] == '-')
		)
	{
		return 1;
	}
    else
    {
        return 0;
    }
}
static int IsREFPROP(std::string Ref)
{
    if (!Ref.compare(0,8,"REFPROP-"))
        return 1;
    else
        return 0;
}
EXPORT_CODE int CONVENTION IsFluidType(char *Ref, char *Type)
{
	pFluid = Fluids.get_fluid(Ref);

	if (IsBrine(Ref)){
		if (!strcmp(Type,"Brine")){
			return 1;
		}
		else{
			return 0;
		}
	}
	else if (IsIncompressibleLiquid(Ref)){
		if (!strcmp(Type,"Liquid")){
			return 1;
		}
		else{
			return 0;
		}
	}
	else if (IsREFPROP(Ref)){
		if (!strcmp(Type,"PureFluid")){
			return 1;
		}
		else{
			return 0;
		}
	}
	else if (!pFluid->pure()){
		if (!strcmp(Type,"PseudoPure") || !strcmp(Type,"PseudoPureFluid")){
			return 1;
		}
		else{
			return 0;
		}
	}
	else if (pFluid->pure()){
		if (!strcmp(Type,"PureFluid")){
			return 1;
		}
		else{
			return 0;
		}
	}
    else
    {
        return 0;
    }
}

EXPORT_CODE double CONVENTION rhosatL_anc(char* Fluid, double T)
{
	try{
		// Try to load the CoolProp Fluid
		pFluid = Fluids.get_fluid(std::string(Fluid));
		return pFluid->rhosatL(T);
	}
	catch(NotImplementedError){
		return -_HUGE;
	}
}
EXPORT_CODE double CONVENTION rhosatV_anc(char* Fluid, double T)
{
	try{
		// Try to load the CoolProp Fluid
		pFluid = Fluids.get_fluid(std::string(Fluid));
		return pFluid->rhosatV(T);
	}
	catch(NotImplementedError){
		return -_HUGE;
	}
}
EXPORT_CODE double CONVENTION psatL_anc(char* Fluid, double T)
{
	try{
		// Try to load the CoolProp Fluid
		pFluid = Fluids.get_fluid(std::string(Fluid));
		return pFluid->psatL_anc(T);
	}
	catch(NotImplementedError){
		return -_HUGE;
	}
}
EXPORT_CODE double CONVENTION psatV_anc(char* Fluid, double T)
{
	try{
		// Try to load the CoolProp Fluid
		pFluid = Fluids.get_fluid(std::string(Fluid));
		return pFluid->psatV_anc(T);
	}
	catch(NotImplementedError){
		return -_HUGE;
	}
}

double _T_hp_secant(std::string Ref, double h, double p, double T_guess)
{
    double x1=0,x2=0,x3=0,y1=0,y2=0,eps=1e-8,change=999,f=999,T=300;
    int iter=1;

    while ((iter<=3 || fabs(f)>eps) && iter<100)
    {
        if (iter==1){x1=T_guess; T=x1;}
        if (iter==2){x2=T_guess+0.1; T=x2;}
        if (iter>2) {T=x2;}
            f=Props("H",'T',T,'P',p,Ref)-h;
        if (iter==1){y1=f;}
        if (iter>1)
        {
            y2=f;
            x3=x2-y2/(y2-y1)*(x2-x1);
            change=fabs(y2/(y2-y1)*(x2-x1));
            y1=y2; x1=x2; x2=x3;
        }
        iter=iter+1;
        if (iter>100)
        {
			throw SolutionError(format("iter %d: T_hp not converging with inputs(%s,%g,%g,%g) value: %0.12g\n",iter,(char*)Ref.c_str(),h,p,T_guess,f));
        }
    }
    return T;
}

EXPORT_CODE void CONVENTION Phase(char *Fluid,double T, double p, char *Phase_str)
{
	strcpy(Phase_str,(char*)Phase(std::string(Fluid),T,p).c_str());
}
EXPORT_CODE long CONVENTION Phase_Tp(char *Fluid,double T, double p, char *Phase_str)
{
	strcpy(Phase_str,(char*)Phase(std::string(Fluid),T,p).c_str());
	return 0;
}
EXPORT_CODE long CONVENTION Phase_Trho(char *Fluid,double T, double rho, char *Phase_str)
{
	strcpy(Phase_str,(char*)Phase_Trho(std::string(Fluid),T,rho).c_str());
	return 0;
}

std::string Phase_Trho(std::string Fluid, double T, double rho)
{
	try{
		// Try to load the CoolProp Fluid
		pFluid = Fluids.get_fluid(Fluid);
		double pL,pV,rhoL,rhoV;
		return pFluid->phase_Trho(T,rho, &pL, &pV, &rhoL, &rhoV);
	}
	catch(NotImplementedError){
		return std::string("");
	}
}

std::string Phase(std::string Fluid, double T, double p)
{
	try{
		// Try to load the CoolProp Fluid
		pFluid = Fluids.get_fluid(Fluid);
		double pL,pV,rhoL,rhoV;
		return pFluid->phase_Tp(T, p, &pL, &pV, &rhoL, &rhoV);
	}
	catch(NotImplementedError){
		return std::string("");
	}
}

std::string Phase_Tp(std::string Fluid, double T, double p)
{
	return Phase(Fluid,T,p);
}

// All the function interfaces that point to the single-input Props function
EXPORT_CODE double CONVENTION Props1(char* Ref, char * Output)
{
	// Redirect to the Props function - should have called it Props1 from the outset
	return Props(Ref, Output);
}
double Props1(std::string Ref, std::string Output)
{
	// Redirect to the Props function - should have called it Props1 from the outset
	return Props((char*)Ref.c_str(), (char*)Output.c_str());
}
double Props(std::string Ref,std::string Output)
{
	return Props((char*)Ref.c_str(), (char*)Output.c_str());
}

double Props(char *Fluid, char *Output)
{
	try{	
		// Try to load the CoolProp Fluid
		pFluid = Fluids.get_fluid(Fluid);
		if (pFluid == NULL){
			// It didn't load properly.  Perhaps it is a REFPROP fluid.
			try{
				if (!strcmp(Output,"Ttriple"))
					return Props('R','T',0,'P',0,Fluid);
				else if (!strcmp(Output,"Tcrit"))
					return Props('B','T',0,'P',0,Fluid);
				else if (!strcmp(Output,"pcrit"))
					return Props('E','T',0,'P',0,Fluid);
				else if (!strcmp(Output,"Tmin"))
					return Props('t','T',0,'P',0,Fluid);
				else if (!strcmp(Output,"molemass"))
					return Props('M','T',0,'P',0,Fluid);
				else if (!strcmp(Output,"rhocrit"))
					return Props('N','T',0,'P',0,Fluid);
				else if (!strcmp(Output,"accentric"))
					return Props('w','T',0,'P',0,Fluid);
				else 
					throw ValueError(format("Output parameter \"%s\" is invalid",Output));
			}
			// Catch any error that subclasses the std::exception
			catch(std::exception &e){
				err_string = std::string("CoolProp error: ").append(e.what());
				std::cout << err_string <<std::endl;
				return _HUGE;
			}
		}
		else{
			// Fluid was loaded successfully - it is a CoolProp fluid
			if (!strcmp(Output,"Ttriple"))
				return pFluid->params.Ttriple;
			else if (!strcmp(Output,"ptriple"))
				return pFluid->params.ptriple;
			else if (!strcmp(Output,"Tcrit"))
				return pFluid->crit.T;
			else if (!strcmp(Output,"Tmin"))
				return pFluid->limits.Tmin;
			else if (!strcmp(Output,"pcrit"))
				return pFluid->crit.p;
			else if (!strcmp(Output,"rhocrit"))
				return pFluid->crit.rho;
			else if (!strcmp(Output,"molemass"))
				return pFluid->params.molemass;
			else if (!strcmp(Output,"accentric"))
				return pFluid->params.accentricfactor;
			else{
				throw ValueError(format("Output parameter \"%s\" is invalid",Output));
				return _HUGE;
			}
		}
	}
	// Catch any error that subclasses the std::exception
	catch(std::exception &e){
		err_string = std::string("CoolProp error: ").append(e.what());
		std::cout << err_string <<std::endl;
		return _HUGE;
	}
	catch(...)
	{
		std::cout << "Indeterminate error" << std::endl;
		return _HUGE;
	}
}

EXPORT_CODE double CONVENTION Props(char *Output,char Name1, double Prop1, char Name2, double Prop2, char * Ref)
{
	// Go to the std::string, std::string version
	return Props(std::string(Output),Name1,Prop1,Name2,Prop2,std::string(Ref));
}
double Props(char Output,char Name1, double Prop1, char Name2, double Prop2, char* Ref)
{
	// Go to the std::string, std::string version
	return Props(std::string(1,Output),Name1,Prop1,Name2,Prop2,std::string(Ref));
}

double Props(std::string Output,char Name1, double Prop1, char Name2, double Prop2, std::string Ref)
{
	// In this function the error catching happens;
	try{
		return _Props(Output,std::string(1,Name1),Prop1,std::string(1,Name2),Prop2,Ref);
	}
	catch(std::exception &e){
		err_string=std::string("CoolProp error: ").append(e.what());
		return _HUGE;
	}
	catch(...){
		err_string=std::string("CoolProp error: Indeterminate error");
		return _HUGE;
	}
}

// Make this a wrapped function so that error bubbling can be done properly
double _Props(std::string Output,std::string Name1, double Prop1, std::string Name2, double Prop2, std::string Ref)
{

	if (debug()>5){
		std::cout<<__FILE__<<": "<<Output<<","<<Name1<<","<<Prop1<<","<<Name2<<","<<Prop2<<","<<Ref<<std::endl;
	}

	if (IsCoolPropFluid(Ref))
	{
		pFluid = Fluids.get_fluid(Ref);
		// Convert all the parameters to integers
		long iOutput = get_param_index(Output);
		if (iOutput<0) 
			throw ValueError(format("Your output key [%s] is not valid. (names are case sensitive)",Output.c_str()));
		long iName1 = get_param_index(std::string(Name1));  
		if (iName1<0) 
			throw ValueError(format("Your input key #1 [%s] is not valid. (names are case sensitive)",Name1.c_str()));
		long iName2 = get_param_index(std::string(Name2));  
		if (iName2<0) 
			throw ValueError(format("Your input key #2 [%s] is not valid. (names are case sensitive)",Name2.c_str()));
		// Call the internal method that uses the parameters converted to longs
		return _CoolProp_Fluid_Props(iOutput,iName1,Prop1,iName2,Prop2,pFluid);
	}
    /* 
    If the fluid name is not actually a refrigerant name, but a string beginning with "REFPROP-",
    then REFPROP is used to calculate the desired property.
    */
    else if (IsREFPROP(Ref))  // First eight characters match "REFPROP-"
    {
        #if defined(__ISWINDOWS__)
		return REFPROP(Output,Name1,Prop1,Name2,Prop2,Ref);
		#else
        throw AttributeError(format("Your refrigerant [%s] is from REFPROP, but REFPROP not supported on this platform",Ref.c_str()));
        return -_HUGE;
        #endif
    }

    // It's a brine, call the brine routine
	else if (IsBrine((char*)Ref.c_str()))
    {
		//Enthalpy and pressure are the inputs
		if ((Name1.c_str()[0]=='H' && Name2.c_str()[0]=='P') || (Name2.c_str()[0]=='H' && Name1.c_str()[0]=='P'))
        {
			if (Name2.c_str()[0]=='H' && Name1.c_str()[0]=='P')
			{
				std::swap(Prop1,Prop2);
				std::swap(Name1,Name2);
			}
			// Start with a guess of 10 K below max temp of fluid
			double Tguess = SecFluids('M',Prop1,Prop2,(char*)Ref.c_str())-10;
			// Solve for the temperature
			double T =_T_hp_secant(Ref,Prop1,Prop2,Tguess);
			// Return whatever property is desired
			return SecFluids(Output[0],T,Prop2,(char*)Ref.c_str());
		}
		else if ((Name1.c_str()[0] == 'T' && Name2.c_str()[0] =='P') || (Name1.c_str()[0] == 'P' && Name2.c_str()[0] == 'T'))
        {
			if (Name1.c_str()[0] =='P' && Name2.c_str()[0] =='T'){
				std::swap(Prop1,Prop2);
			}
			return SecFluids(Output[0],Prop1,Prop2,(char*)Ref.c_str());
		}
		else
		{
			throw ValueError("For brine, inputs must be (order doesnt matter) 'T' and 'P', or 'H' and 'P'");
		}
    }
	// It's an incompressible liquid, call the routine
	else if (IsIncompressibleLiquid((char*)Ref.c_str()))
    {
		//Enthalpy and pressure are the inputs
		if ((Name1.c_str()[0]=='H' && Name2.c_str()[0]=='P') || (Name2.c_str()[0]=='H' && Name1.c_str()[0]=='P'))
        {
			if (Name2.c_str()[0]=='H' && Name1.c_str()[0]=='P')
			{
				std::swap(Prop1,Prop2);
				std::swap(Name1,Name2);
			}
			
			// Solve for the temperature
			double T =_T_hp_secant(Ref,Prop1,Prop2,300);
			// Return whatever property is desired
			return IncompLiquid(get_param_index(Output),T,Prop2,(char*)Ref.c_str());
		}
		else if ((Name1.c_str()[0] == 'T' && Name2.c_str()[0] =='P') || (Name1.c_str()[0] == 'P' && Name2.c_str()[0] == 'T'))
        {
			if (Name1.c_str()[0] =='P' && Name2.c_str()[0] =='T'){
				std::swap(Prop1,Prop2);
			}
			return IncompLiquid(get_param_index(Output),Prop1,Prop2,Ref);
		}
		else
		{
			throw ValueError("For brine, inputs must be (order doesnt matter) 'T' and 'P', or 'H' and 'P'");
		}
    }
	else
	{
		throw ValueError(format("Your fluid name [%s] is not a CoolProp fluid, a REFPROP fluid, a brine or a liquid",Ref.c_str()));
	}
    return 0;
}
double _CoolProp_Fluid_TwoPhaseProps(long iOutput, double Q, Fluid *pFluid, double TL, double TV, double pL, double pV, double rhoL, double rhoV)
{
	// In this function you know you are within the two-phase region, and might be saturated liquid or vapor.
	// You have already carried out the saturation calculations before so you have the saturation temperatures, densities, and pressures
	
	// If you are saturated, just use the saturated density back into the main function and enforce single-phase.
	// In reality it is not single-phase but it is thermodynamically consistent to just plug in the saturated density into the EOS

	// Trivial output
	if (iOutput == iQ)
		return Q;

	if (fabs(Q)<1e-12)
		// Recurse and call Props again with the saturated liquid density
		return _CoolProp_Fluid_Props(iOutput,iT,TL,iD,rhoL,pFluid,true);
	else if (fabs(Q-1)<1e-12)
		// Recurse and call Props again with the saturated vapor density
		return _CoolProp_Fluid_Props(iOutput,iT,TV,iD,rhoV,pFluid,true);
	else{
		if (iOutput==iD){
			return 1/(Q/rhoV+(1-Q)/rhoL);
		}
		else{
			// Quality weighted temperature for pseudo-pure
			double T = Q*TV+(1-Q)*TL;
			// Do a quality-weighted-average of the saturated liquid and vapor properties
			return Q*_CoolProp_Fluid_Props(iOutput,iT,T,iD,rhoV,pFluid,true)+(1-Q)*_CoolProp_Fluid_Props(iOutput,iT,T,iD,rhoL,pFluid,true);
		}
	}
}
double _CoolProp_Fluid_Props(long iOutput, long iName1, double Prop1, long iName2, double Prop2, Fluid *pFluid, bool SinglePhase)
{
	double T,Q,rhoV,rhoL,Value,rho,pL,pV;

	CoolPropStateClass CPS = CoolPropStateClass(pFluid);
	// This private method uses the indices directly for speed

	// Check if it is an output that doesn't require a state input
    // Deal with it and return

	if (iOutput<0)
		throw ValueError("Input key is invalid");
	switch (iOutput)
	{
		case iMM:
		case iPcrit:
		case iTcrit:
		case iTtriple:
		case iRhocrit:
		case iAccentric: 
			return CPS.keyed_output(iOutput);
	}

	/*if (iName1 == iT && Prop1 < pFluid->limits.Tmin) 
		throw ValueError(format("Input temperature to Props function [%f K] is below the fluid minimum temp [%f K]",Prop1,pFluid->limits.Tmin));
	if (iName2 == iT && Prop2 < pFluid->limits.Tmin) 
		throw ValueError(format("Input temperature to Props function [%f K] is below the fluid minimum temp [%f K]",Prop2,pFluid->limits.Tmin));*/

	CPS.update(iName1,Prop1,iName2,Prop2);
	return CPS.keyed_output(iOutput);
}
//	//Surface tension is only a function of temperature
//	if (iOutput == iI){
//		if (iName1 == iT)
//			return pFluid->surface_tension_T(Prop1);
//		else if (iName2 == iT)
//			return pFluid->surface_tension_T(Prop2);
//		else
//			throw ValueError(format("If output is surface tension ['I' or 'SurfaceTension'], Param1 must be temperature"));
//	}
//
//	// In any case, you want to get a (temperature, density) pair
//	if ((iName1 == iT && iName2 == iP) || (iName1 == iP && iName2 == iT))
//	{
//		//Swap values and keys
//		if (iName1 == iP && iName2 == iT){
//			std::swap(Prop1,Prop2);
//			std::swap(iName1,iName2);
//		}
//
//		// Handle trivial outputs
//		if (iOutput == iP) 
//			return Prop2;
//		else if (iOutput == iT) 
//			return Prop1;
//		
//		// Get density as a function of T&p, then call again
//		rho = rho_TP(Prop1,Prop2);
//		
//		if (iOutput == iD){
//			if (debug()>5){
//				std::cout<<__FILE__<<": "<<iOutput<<","<<iName1<<","<<Prop1<<","<<iName2<<","<<Prop2<<","<<pFluid->get_name()<<"="<<rho<<std::endl;
//			}
//			return rho;
//		}
//		else
//		{
//			// Recurse, telling CoolProp that it is single-phase by setting the last parameter to true
//			return _CoolProp_Fluid_Props(iOutput,iName1,Prop1,iD,rho,pFluid,true);
//		}
//	}
//	else if ((iName1 == iT && iName2 == iQ) || (iName1 == iQ && iName2 == iT))
//	{
//		if (iName1 == iQ && iName2 == iT){
//			//Swap values and keys to get order of T, Q
//			std::swap(Prop1,Prop2);
//			std::swap(iName1,iName2);
//		}
//		
//		T = Prop1;
//		Q = Prop2;
//		if (T <= pFluid->params.Ttriple || T > pFluid->crit.T){
//			throw ValueError(format("Your saturation temperature [%f K] is out of range [%f K, %f K]",T,pFluid->params.Ttriple,pFluid->crit.T ));
//		}
//		if (Q>1+10*DBL_EPSILON || Q<-10*DBL_EPSILON){
//			throw ValueError(format("Your quality [%f] is out of range (0, 1)",Q ));
//		}
//		// Get the saturation properties
//		pFluid->saturation_T(Prop1,pFluid->enabled_TTSE_LUT,&pL,&pV,&rhoL,&rhoV);
//		// Find the effective density to use
//		rho=1/(Q/rhoV+(1-Q)/rhoL);
//
//		// Trivial outputs
//		if (iOutput == iP)
//			return Q*pV+(1-Q)*pL;
//		if (iOutput == iQ)
//			return Q;
//		if (iOutput == iD)
//			return 1/(Q/rhoV+(1-Q)/rhoL);
//	
//		// Saturated liquid
//		if (fabs(Q)<10*DBL_EPSILON)
//			return _CoolProp_Fluid_Props(iOutput,iT,Prop1,iD,rhoL,pFluid,true);
//		// Saturated vapor
//		else if (fabs(Q-1)<10*DBL_EPSILON)
//			return _CoolProp_Fluid_Props(iOutput,iT,Prop1,iD,rhoV,pFluid,true);
//		// Neither saturated liquid not saturated vapor
//		else
//			return Q*_CoolProp_Fluid_Props(iOutput,iT,Prop1,iD,rhoV,pFluid,true)+(1-Q)*_CoolProp_Fluid_Props(iOutput,iT,Prop1,iD,rhoL,pFluid,true);
//	}
//	else if ((iName1 == iT && iName2 == iD) || (iName1 == iD && iName2 == iT))
//	{
//		if (iName1 == iD && iName2 == iT)
//		{
//			//Swap values and keys to get T,D
//			std::swap(Prop1,Prop2);
//			std::swap(iName1,iName2);
//		}
//		T=Prop1;
//		rho=Prop2;
//		// Trivial output
//		if (iOutput == iD)
//			return rho;
//		else if (iOutput == iT)
//			return T;
//
//		rho = Prop2; 
//		double pL,pV,rhoL,rhoV;
//		// If SinglePhase is true here it will shortcut and not do the saturation 
//		// calculation, saving a lot of computational effort
//		if (!SinglePhase && !global_SinglePhase && !pFluid->phase_Trho(T,rho,&pL,&pV,&rhoL,&rhoV).compare("Two-Phase"))
//		{
//			double Q = (1/rho-1/rhoL)/(1/rhoV-1/rhoL);
//			return _CoolProp_Fluid_TwoPhaseProps(iOutput,Q,pFluid,T,T,pL,pV,rhoL,rhoV);
//		}
//		else
//		{
//			switch (iOutput)
//			{
//				case iP:
//					Value=pFluid->pressure_Trho(T,rho);
//					break;
//				case iH:
//					Value=pFluid->enthalpy_Trho(T,rho);
//					break;
//				case iS:
//					Value=pFluid->entropy_Trho(T,rho);
//					break;
//				case iU:
//					Value=pFluid->internal_energy_Trho(T,rho);
//					break;
//				case iC:
//					Value=pFluid->specific_heat_p_Trho(T,rho);
//					break;
//				case iC0:
//					Value=pFluid->specific_heat_p_ideal_Trho(T);
//					break;
//				case iO:
//					Value=pFluid->specific_heat_v_Trho(T,rho);
//					break;
//				case iA:
//					Value=pFluid->speed_sound_Trho(T,rho);
//					break;
//				case iG:
//					Value=pFluid->gibbs_Trho(T,rho);
//					break;
//				case iV:
//					Value=pFluid->viscosity_Trho(T,rho);
//					break;
//				case iL:
//					Value=pFluid->conductivity_Trho(T,rho);
//					break;
//				case iDpdT:
//					Value=pFluid->dpdT_Trho(T,rho);
//					break;
//				case iDrhodT_p:
//					Value=pFluid->drhodT_p_Trho(T,rho);
//					break;
//				case iQ:
//					throw ValueError(format("Quality is not a valid output for single-phase inputs"));
//				default:
//					throw ValueError(format("Invalid Output index: %d ",iOutput));
//					return _HUGE;
//			}
//		}
//		if (debug()>5){
//			std::cout<<__FILE__<<__LINE__<<": global_SinglePhase is "<<global_SinglePhase<<std::endl;
//			std::cout<<__FILE__<<__LINE__<<": "<<iOutput<<","<<iName1<<","<<Prop1<<","<<iName2<<","<<Prop2<<","<<pFluid->get_name()<<"="<<Value<<std::endl;
//		}
//        return Value;
//	}
//    else if ((iName1 == iP && iName2 == iQ) || (iName1 == iQ && iName2 == iP))
//    {
//		if (iName1 == iQ)
//		{
//			std::swap(Prop1,Prop2);
//		}
//		if (Prop1 <= pFluid->params.ptriple || Prop1 > pFluid->crit.p){
//			throw ValueError(format("Your saturation pressure [%f K] is out of range [%f kPa, %f kPa]",Prop1,pFluid->params.ptriple,pFluid->crit.p ));
//		}
//		if (Prop2>1+10*DBL_EPSILON || Prop2<-10*DBL_EPSILON){
//			throw ValueError(format("Your quality [%f] is out of range (0, 1)",Prop2 ));
//		}
//
//		// Do the saturation call
//		double rhoL, rhoV;
//        T=pFluid->Tsat(Prop1,Prop2,0,pFluid->enabled_TTSE_LUT, &rhoL, &rhoV);
//		// Saturated liquid
//		if (fabs(Prop2) < 10*DBL_EPSILON)
//		{
//			return _CoolProp_Fluid_Props(iOutput,iT,T,iD,rhoL,pFluid,true);
//		}
//		// Saturated vapor
//		else if (fabs(Prop2-1) < 10*DBL_EPSILON)
//		{
//			return _CoolProp_Fluid_Props(iOutput,iT,T,iD,rhoV,pFluid,true);
//		}
//		// Neither saturated liquid not saturated vapor
//		else
//		{
//			return _CoolProp_Fluid_Props(iOutput,iT,T,iQ,Prop2,pFluid);
//		}
//    }
//    else if ((iName1 == iH && iName2 == iP) || (iName1 == iP && iName2 == iH))
//    {
//		if (iName1 == iP && iName2 == iH){
//			std::swap(Prop1,Prop2);
//		}
//		double h = Prop1;
//		double p = Prop2;
//		double TsatL, TsatV;
//		// Actually call the h,p routine to find temperature, density (and saturation densities if they get calculated)
//    	pFluid->temperature_ph(p, h, &T, &rho, &rhoL, &rhoV, &TsatL, &TsatV);
//
//		// Check if it is two-phase - if so, call the two-phase routine
//		if (p < 0.999*pFluid->crit.p && rho > rhoV && rho < rhoL){
//			// It's two-phase
//			double Q = (1/rho-1/rhoL)/(1/rhoV-1/rhoL);
//			
//			// Handle the trivial outputs right now
//			if (iOutput == iQ)
//				return Q;
//			else if (iOutput == iD)
//				return 1/(Q/rhoV+(1-Q)/rhoL);
//
//			// Go into the two-phase routine
//			return _CoolProp_Fluid_TwoPhaseProps(iOutput,Q,pFluid,T,T,p,p,rhoL,rhoV);
//		}
//		else{
//			// It's not two phase, use the density and temperature to find the state point
//			// Non two-phase is strictly enforced here by passing the true in as the last parameter
//			return _CoolProp_Fluid_Props(iOutput,iT,T,iD,rho,pFluid,true);
//		}
//    }
//	else if ((iName1 == iS && iName2 == iP) || (iName1 == iP && iName2 == iS))
//    {
//		if (iName1 == iP && iName2 == iS){
//			std::swap(Prop1,Prop2);
//		}
//		double s = Prop1;
//		double p = Prop2;
//		double TsatL, TsatV;
//		
//		// Actually call the s,p routine to find temperature, density (and saturation densities if they get calculated)
//    	pFluid->temperature_ps(p, s, &T, &rho, &rhoL, &rhoV, &TsatL, &TsatV);
//
//		// Check if it is two-phase - if so, call the two-phase routine
//		if (p < 0.999*pFluid->crit.p && rho > rhoV && rho < rhoL){
//			// It's two-phase
//			double Q = (1/rho-1/rhoL)/(1/rhoV-1/rhoL);
//			if (iOutput == iQ)
//				return Q;
//			return _CoolProp_Fluid_TwoPhaseProps(iOutput,Q,pFluid,T,T,p,p,rhoL,rhoV);
//		}
//		else{
//			// It's not two phase, use the density and temperature to find the state point
//			// Non two-phase is strictly enforced here by passing the true in as the last parameter since we know for sure it is not two-phase
//			return _CoolProp_Fluid_Props(iOutput,iT,T,iD,rho,pFluid,true);
//		}
//    }
//    else
//    {
//		throw ValueError(format("Not a valid pair of input keys %d,%d and output key %d",iName1,iName2,iOutput));
//    }
//}
EXPORT_CODE double CONVENTION IProps(long iOutput, long iName1, double Prop1, long iName2, double Prop2, long iFluid)
{
	pFluid = Fluids.get_fluid(iFluid);
	// Didn't work
	if (pFluid == NULL){
		err_string=std::string("CoolProp error: ").append(format("%d is an invalid fluid index to IProps",iFluid));
		return _HUGE;
	}
	else{
		// In this function the error catching happens;
		try{
			return _CoolProp_Fluid_Props(iOutput,iName1,Prop1,iName2,Prop2,pFluid);
		}
		catch(std::exception &e){
			err_string=std::string("CoolProp error: ").append(e.what());
			return _HUGE;
		}
		catch(...){
			err_string=std::string("CoolProp error: Indeterminate error");
			return _HUGE;
		}
	}
}

EXPORT_CODE double CONVENTION K2F(double T)
{ return T * 9 / 5 - 459.67; }

EXPORT_CODE double CONVENTION F2K(double T_F)
{ return (T_F + 459.67) * 5 / 9;}

EXPORT_CODE void CONVENTION PrintSaturationTable(char *FileName, char * Ref, double Tmin, double Tmax)
{
    double T;
    FILE *f;
    f=fopen(FileName,"w");
    fprintf(f,"T,pL,pV,rhoL,rhoV,uL,uV,hL,hV,sL,sV,cvL,cvV,cpL,cpV,kL,kV,muL,muV\n");
    fprintf(f,"[K],[kPa],[kPa],[kg/m^3],[kg/m^3],[kJ/kg],[kJ/kg],[kJ/kg],[kJ/kg],[kJ/kg-K],[kJ/kg-K],[kJ/kg-K],[kJ/kg-K],[kJ/kg-K],[kJ/kg-K],[kW/m-K],[kW/m-K],[Pa-s],[Pa-s]\n");
    fprintf(f,"--------------------------------------------------------------------------\n");

    for (T=Tmin;T<Tmax;T+=1.0)
    {
    fprintf(f,"%0.3f,",T);
    fprintf(f,"%0.6f,",Props('P','T',T,'Q',0,Ref));
    fprintf(f,"%0.6f,",Props('P','T',T,'Q',1,Ref));
    fprintf(f,"%0.6f,",Props('D','T',T,'Q',0,Ref));
    fprintf(f,"%0.6f,",Props('D','T',T,'Q',1,Ref));
    fprintf(f,"%0.6f,",Props('U','T',T,'Q',0,Ref));
    fprintf(f,"%0.6f,",Props('U','T',T,'Q',1,Ref));
    fprintf(f,"%0.6f,",Props('H','T',T,'Q',0,Ref));
    fprintf(f,"%0.6f,",Props('H','T',T,'Q',1,Ref));
    fprintf(f,"%0.6f,",Props('S','T',T,'Q',0,Ref));
    fprintf(f,"%0.6f,",Props('S','T',T,'Q',1,Ref));
    fprintf(f,"%0.6f,",Props('O','T',T,'Q',0,Ref));
    fprintf(f,"%0.6f,",Props('O','T',T,'Q',1,Ref));
    fprintf(f,"%0.6f,",Props('C','T',T,'Q',0,Ref));
    fprintf(f,"%0.6f,",Props('C','T',T,'Q',1,Ref));
    fprintf(f,"%0.9f,",Props('L','T',T,'Q',0,Ref));
    fprintf(f,"%0.9f,",Props('L','T',T,'Q',1,Ref));
    fprintf(f,"%0.6g,",Props('V','T',T,'Q',0,Ref));
    fprintf(f,"%0.6g,",Props('V','T',T,'Q',1,Ref));
    fprintf(f,"\n");
    }
    fclose(f);
}

EXPORT_CODE void CONVENTION FluidsList(char* str)
{
	str=(char*)FluidsList().c_str();
	return;
}
std::string FluidsList()
{
	return Fluids.FluidList();
}

EXPORT_CODE double CONVENTION DerivTerms(char *Term, double T, double rho, char * Ref)
{
	pFluid=Fluids.get_fluid(Ref);
	return DerivTerms(Term,T,rho,pFluid);
}

double DerivTerms(char *Term, double T, double rho, Fluid * pFluid)
{
	return DerivTerms(Term,T,rho,pFluid,false,false);
}

/// Calculate some interesting derivatives
/// @param SinglePhase true if it is known to be single phase
/// @param TwoPhase true if it is known to be two phase
/// If phase is not known, set both SinglePhase and TwoPhase to false
double DerivTerms(char *Term, double T, double rho, Fluid * pFluid, bool SinglePhase, bool TwoPhase)
{
    double rhoc =pFluid->reduce.rho;
	double delta=rho/rhoc;
	double tau=pFluid->reduce.T/T;
	double R=pFluid->R();
	double dtau_dT = -pFluid->reduce.T/T/T;

	CoolPropStateClass CPS = CoolPropStateClass(pFluid);
	CPS.update(iT,T,iD,rho);

	if (!strcmp(Term,"dpdT") || !strcmp(Term,"dpdT|rho")){
		return CPS.dpdT_constrho();
	}
    else if (!strcmp(Term,"dpdrho") || !strcmp(Term,"dpdrho|T")){
		return CPS.dpdrho_constT();
	}
	else if (!strcmp(Term,"dhdT") || !strcmp(Term,"dhdT|rho")){
		return CPS.dhdT_constrho();
	}
	else if (!strcmp(Term,"dhdrho") || !strcmp(Term,"dhdrho|T")){
		return CPS.dhdrho_constT();
	}
	else if (!strcmp(Term,"drhodT|p")){
		return CPS.drhodT_constp();
	}
	else if (!strcmp(Term,"drhodh|p")){
		return CPS.drhodh_constp();
	}
	else if (!strcmp(Term,"drhodp|h")){
		return CPS.drhodp_consth();
	}
	else if (!strcmp(Term,"dhdp|rho") || !strcmp(Term,"dhdp|v"))
	{
		// All the terms are re-calculated here in order to cut down on the number of calls
		double dphir_dDelta = pFluid->dphir_dDelta(tau,delta);	
		double d2phir_dDelta_dTau = pFluid->d2phir_dDelta_dTau(tau,delta);
		double d2phir_dDelta2 = pFluid->d2phir_dDelta2(tau,delta);
		
		double dpdrho = R*T*(1+2*delta*dphir_dDelta+delta*delta*d2phir_dDelta2);
		double dpdT = R*rho*(1+delta*dphir_dDelta-delta*tau*d2phir_dDelta_dTau);
		double cp = -tau*tau*R*(pFluid->d2phi0_dTau2(tau,delta)+pFluid->d2phir_dTau2(tau,delta))+T/rho/rho*(dpdT*dpdT)/dpdrho;
		double drhodT = -dpdT/dpdrho;
		return -cp/dpdrho/drhodT-T*drhodT*(-1/pow(rho,2))+1/rho;
	}
	else if (!strcmp(Term,"Z"))
	{
		return 1+delta*pFluid->dphir_dDelta(tau,delta);
	}
	else if (!strcmp(Term,"dZ_dDelta"))
    {
        return delta*pFluid->d2phir_dDelta2(tau,delta)+pFluid->dphir_dDelta(tau,delta);
    }
	else if (!strcmp(Term,"dZ_dTau"))
    {
        return delta*pFluid->d2phir_dDelta_dTau(tau,delta);
    }
	else if (!strcmp(Term,"B"))
	{
		// given by B*rhoc=lim(delta --> 0) [dphir_ddelta(tau)]
		return 1.0/rhoc*pFluid->dphir_dDelta(tau,1e-12);
	}
	else if (!strcmp(Term,"dBdT"))
	{
		return 1.0/rhoc*pFluid->d2phir_dDelta_dTau(tau,1e-12)*dtau_dT;
	}
	else if (!strcmp(Term,"C"))
	{
		// given by C*rhoc^2=lim(delta --> 0) [d2phir_dDelta2(tau)]
		return 1.0/(rhoc*rhoc)*pFluid->d2phir_dDelta2(tau,1e-12);
	}
	else if (!strcmp(Term,"dCdT"))
	{
		return 1.0/(rhoc*rhoc)*pFluid->d3phir_dDelta2_dTau(tau,1e-12)*dtau_dT;
    }
	else if (!strcmp(Term,"phir"))
    {
        return pFluid->phir(tau,delta);
    }
	else if (!strcmp(Term,"dphir_dTau"))
    {
        return pFluid->dphir_dTau(tau,delta);
    }
	else if (!strcmp(Term,"dphir_dDelta"))
    {
        return pFluid->dphir_dDelta(tau,delta);
    }
	else if (!strcmp(Term,"d2phir_dTau2"))
    {
        return pFluid->d2phir_dTau2(tau,delta);
    }
	else if (!strcmp(Term,"d2phir_dDelta2"))
    {
        return pFluid->d2phir_dDelta2(tau,delta);
    }
	else if (!strcmp(Term,"d2phir_dDelta_dTau"))
    {
        return pFluid->d2phir_dDelta_dTau(tau,delta);
    }
	else if (!strcmp(Term,"d3phir_dDelta3"))
    {
        return pFluid->d3phir_dDelta3(tau,delta);
    }
	else if (!strcmp(Term,"d3phir_dDelta2_dTau"))
    {
        return pFluid->d3phir_dDelta2_dTau(tau,delta);
    }
	else if (!strcmp(Term,"d3phir_dDelta_dTau2"))
    {
        return pFluid->d3phir_dDelta_dTau2(tau,delta);
    }
	else if (!strcmp(Term,"d3phir_dTau3"))
    {
        return pFluid->d3phir_dTau3(tau,delta);
    }
	else if (!strcmp(Term,"phi0"))
    {
        return pFluid->phi0(tau,delta);
    }
    else if (!strcmp(Term,"dphi0_dTau"))
    {
        return pFluid->dphi0_dTau(tau,delta);
    }
	else if (!strcmp(Term,"d2phi0_dTau2"))
    {
        return pFluid->d2phi0_dTau2(tau,delta);
    }
	else if (!strcmp(Term,"dphi0_dDelta"))
    {
        return pFluid->dphi0_dDelta(tau,delta);
    }
	else if (!strcmp(Term,"d2phi0_dDelta2"))
    {
        return pFluid->d2phi0_dDelta2(tau,delta);
    }
	else if (!strcmp(Term,"d2phi0_dDelta_dTau"))
    {
        return pFluid->d2phi0_dDelta_dTau(tau,delta);
    }
	else if (!strcmp(Term,"d3phi0_dTau3"))
    {
        return pFluid->d3phi0_dTau3(tau,delta);
    }
	else if (!strcmp(Term,"IsothermalCompressibility")){
		return CPS.isothermal_compressibility();
	}
	else
	{
		printf("Sorry DerivTerms is a work in progress, your derivative term [%s] is not available!!",Term);
		return _HUGE;
	}
}
std::string get_EOSReference(std::string Ref)
{
	pFluid=Fluids.get_fluid(Ref);
	if (pFluid!=NULL)
	{
		return pFluid->get_EOSReference();
	}
	else
		return std::string("");
}
std::string get_TransportReference(std::string Ref)
{
	pFluid=Fluids.get_fluid(Ref);
	if (pFluid!=NULL)
	{
		return pFluid->get_TransportReference();
	}
	else
		return std::string("");
}
EXPORT_CODE void CONVENTION get_aliases(char* Ref, char *aliases)
{
	strcpy(aliases, get_aliases(std::string(Ref)).c_str());
}
std::string get_aliases(std::string Ref)
{
	pFluid=Fluids.get_fluid(Ref);
	std::string s;
	
	std::vector<std::string> v = pFluid->get_aliases();
	for (unsigned long i = 0; i< v.size(); i++){
		if (i==0)
		{
			s = v[i];
		}
		else
		{
			s += ", " + v[i];
		}
	}
	return s;
}
EXPORT_CODE void CONVENTION get_REFPROPname(char* Ref, char * str)
{
	str= (char*)get_REFPROPname(std::string(Ref)).c_str();
}
std::string get_REFPROPname(std::string Ref)
{
	pFluid=Fluids.get_fluid(Ref);

	if ( (pFluid->get_REFPROPname()).size()!=0 ){
		return pFluid->get_REFPROPname();
	}
	else{
		return pFluid->get_name();
	}
}
