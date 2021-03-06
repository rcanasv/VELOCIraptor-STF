/*! \file ui.cxx
 *  \brief user interface
 */

#include "baryoniccontent.h"
#include <algorithm>

///routine to get arguments from command line
void GetArgs(int argc, char *argv[], Options &opt)
{
    int option;
    int NumArgs = 0;
    int configflag=0;
    while ((option = getopt(argc, argv, ":C:i:s:h:S:o:n:r:")) != EOF)
    {
        switch(option)
        {
            case 'C':
                opt.configname = optarg;
                configflag=1;
                NumArgs += 2;
                break;
            case 'i':
                opt.fname = optarg;
                NumArgs += 2;
                break;
            case 's':
                opt.nfiles = atoi(optarg);
                NumArgs += 2;
                break;
            case 'h':
                opt.halocatname = optarg;
                NumArgs += 2;
                break;
            case 'S':
                opt.nfileshalocat = atoi(optarg);
                NumArgs += 2;
                break;
            case 'o':
                opt.outname = optarg;
                NumArgs += 2;
                break;
            case 'n':
                opt.Neff = atol(optarg);
/*                opt.Neff=opt.Neff*opt.Neff*opt.Neff;
#if defined(GASON)
                opt.Neff*=2.0;
#endif*/
                NumArgs += 2;
                break;
            case 'r':
                opt.reframe = atoi(optarg);
                NumArgs += 2;
                break;
            case '?':
                usage();
        }
    }
    if(configflag){
        cout<<"Reading config file and overriding all command line passed values"<<endl;
        GetParamFile(opt);
    }
#ifdef USEMPI
    MPI_Barrier(MPI_COMM_WORLD);
#endif
    ConfigCheck(opt);
}

///Outputs the usage to stdout
void usage(void)
{
    Options opt;
#ifdef USEMPI
    if (ThisTask==0) {
#endif
    cerr<<"USAGE:\n";
    cerr<<"\n";
    cerr<<"-i <particle data input file>\n";
    cerr<<"-s <number of files per snapshot for gadget input [1 default], 0 for tipsy >\n";
    cerr<<"-h <halo catalog input file>\n";
    cerr<<"-S <number of files per halo catalog [1] >\n";
    cerr<<"-n <effective resolution [-1] make >0 to use, useful for zoom simulations >\n";
    cerr<<"-C <configuration file> \n";
    cerr<<"-o <output file name> \n";
#ifdef USEMPI
    }
    MPI_Finalize();
#endif
    exit(1);
}

///Read parameters from a parameter file
///\todo still more parameters that can be adjusted
void GetParamFile(Options &opt)
{
    string line,sep="=";
    string tag,val;
    char buff[1024],*pbuff,tbuff[1024],vbuff[1024],fname[1024];
    unsigned j,k;
    fstream paramfile,cfgfile;
    if (!FileExists(opt.configname)){
            cerr<<"Config file does not exist or can't be read, terminating"<<endl;
#ifdef USEMPI
            MPI_Finalize();
#endif
            exit(9);
    }
    paramfile.open(opt.configname, ios::in);
    sprintf(fname,"%s.cfg",opt.outname);
    cfgfile.open(fname, ios::out);
    paramfile.clear();
    paramfile.seekg(0, ios::beg);
    if (paramfile.is_open())
    {
        while (paramfile.good()){
            getline(paramfile,line);
            //if line is not commented out or empty
            if (line[0]!='#'&&line.length()!=0) {
                if (j=line.find(sep)){
                    //clean up string
                    tag=line.substr(0,j);
                    strcpy(buff, tag.c_str());
                    pbuff=strtok(buff," ");
                    strcpy(tbuff, pbuff);
                    val=line.substr(j+1,line.length()-(j+1));
                    strcpy(buff, val.c_str());
                    pbuff=strtok(buff," ");
                    if (pbuff==NULL) continue;
                    strcpy(vbuff, pbuff);
                    cfgfile<<tbuff<<"="<<vbuff<<endl;
                    //change quantities
                    if (strcmp(tbuff, "Quant_values")==0) {
                        //parse vbuff;
                        string s = vbuff;
                        string delim(",");
                        size_t pos;
                        int icount=0;
                        //remove enclosing brackets
                        s.erase(std::remove(s.begin(), s.end(), '['),s.end());
                        s.erase(std::remove(s.begin(), s.end(), ']'),s.end());
                        //get numbers
                        while ((pos = s.find(delim)) != string::npos){
                            opt.quants[icount++]= ::atof((s.substr(0, pos)).c_str());
                            s.erase(0, pos + delim.length());
                            if (icount>=MAXNQUANTS) {
                                cout<<"ERROR: exceeded maximum number of quantiles"<<endl;
#ifdef USEMPI
                                MPI_Finalize();
#endif
                                exit(9);
                            }
                        }
                        opt.nquants=icount;
                    }
                    //units, cosmology
                    else if (strcmp(tbuff, "Length_unit")==0)
                        opt.L = atof(vbuff);
                    else if (strcmp(tbuff, "Velocity_unit")==0)
                        opt.V = atof(vbuff);
                    else if (strcmp(tbuff, "Mass_unit")==0)
                        opt.M = atof(vbuff);
                    else if (strcmp(tbuff, "Pressure_unit")==0)
                        opt.PUnit = atof(vbuff);
                    else if (strcmp(tbuff, "Temperature_unit")==0)
                        opt.TUnit = atof(vbuff);
                    else if (strcmp(tbuff, "Hubble_unit")==0)
                        opt.H = atof(vbuff);
                    else if (strcmp(tbuff, "Gravity")==0)
                        opt.G = atof(vbuff);
                    else if (strcmp(tbuff, "Period")==0)
                        opt.p = atof(vbuff);
                    else if (strcmp(tbuff, "Scale_factor")==0)
                        opt.a = atof(vbuff);
                    else if (strcmp(tbuff, "h_val")==0)
                        opt.h = atof(vbuff);
                    else if (strcmp(tbuff, "Omega_m")==0)
                        opt.Omega_m = atof(vbuff);
                    else if (strcmp(tbuff, "Omega_Lambda")==0)
                        opt.Omega_Lambda = atof(vbuff);
                    else if (strcmp(tbuff, "Critical_density")==0)
                        opt.rhobg = atof(vbuff);
                    else if (strcmp(tbuff, "Virial_density")==0)
                        opt.virlevel = atof(vbuff);
                    //useful for zoom simulations
                    else if (strcmp(tbuff, "Effective_resolution")==0) {
                        opt.Neff = atol(vbuff);
/*                        opt.Neff=opt.Neff*opt.Neff*opt.Neff;
#if defined(GASON)
                        opt.Neff*=2.0;
#endif*/
                    }
                    //unbinding
                    else if (strcmp(tbuff, "Softening_length")==0)
                        opt.uinfo.eps = atof(vbuff);
                    //other options
                    else if (strcmp(tbuff, "Verbose")==0)
                        opt.verbose = atoi(vbuff);
                    //input options
                    else if (strcmp(tbuff, "Separate_velociraptor_input_files")==0)
                        opt.iseparatefiles = atoi(vbuff);
                    else if (strcmp(tbuff, "Binary_velociraptor_input_files")==0)
                        opt.ibinaryin = atoi(vbuff);
                    else if (strcmp(tbuff, "Binary_output")==0)
                        opt.ibinaryout = atoi(vbuff);
                }
            }
        }
        paramfile.close();
        cfgfile.close();
    }
}

inline void ConfigCheck(Options &opt)
{
    if (opt.fname==NULL||opt.outname==NULL||opt.halocatname==NULL){
#ifdef USEMPI
    if (ThisTask==0)
#endif
        cerr<<"Must provide input and output file names\n";
#ifdef USEMPI
        MPI_Finalize();
#endif
        exit(8);
    }
#ifdef USEMPI
    if (ThisTask==0)
#endif
    cout<<"Units: L="<<opt.L<<", M="<<opt.M<<", V="<<opt.V<<", G="<<opt.G<<endl;
	if (opt.reframe==IPOTFRAME) opt.ipotcalc=0;
}

