//
// $Id$
//

#include "FileHelper.h"
#include "ConfigMgr.h"
#include "Exception.h"

#include <libgen.h>
#include <stdio.h>
#include <sys/stat.h>

#include <map>
#include <iostream>
#include <fstream>

using namespace std;

namespace avg {

string getPath(const string& Filename)
{
    if (Filename.length() > 0 && Filename.at(Filename.length()-1) == '/') {
        return Filename;
    }
    char * pszBuffer = strdup(Filename.c_str());

    string DirName(dirname(pszBuffer));
    free(pszBuffer);

    DirName += "/";
    
    return DirName;
}

bool fileExists(const std::string& FileName) {
    struct stat myStat;
    return stat(FileName.c_str(), &myStat) != -1;
}

string findFile (const string & sFilename, const string & sPathOptionSubsys,
        const string & sPathOptionName, const string & sCurJSFilename)
{
    static map<string, string> SearchPaths;
    map<string, string>::iterator it;
    it = SearchPaths.find(sPathOptionName);
    string sPath;
    if (it == SearchPaths.end()) {
        const string* psPath;
        if (sPathOptionSubsys == "") {
            psPath = ConfigMgr::get()->getGlobalOption(sPathOptionName);
        } else {
            psPath = ConfigMgr::get()->getOption(sPathOptionSubsys, 
                    sPathOptionName);
        }
        if (psPath) {
            sPath = *psPath;
            sPath += ";";
        } else {
            cerr << "Warning: " << sPathOptionName << " not set." << endl;
        }
        sPath += getPath(sCurJSFilename);
        SearchPaths[sPathOptionName] = sPath;
    } else {
        sPath = (*it).second;
    }
    
    string SearchPathLeft = sPath;
    static const char * pDelimiters = ";:";

    string::size_type EndPos = SearchPathLeft.find_first_of(pDelimiters);

    while (EndPos != std::string::npos) {        
        string Path = SearchPathLeft.substr(0, EndPos);

        if (Path.at(Path.size() - 1) != '/') {
            Path += "/";
        }
        string FileWithPath = Path + sFilename;            
        if (fileExists(FileWithPath)) {
            return FileWithPath;
        }

        SearchPathLeft = SearchPathLeft.substr(EndPos + 1);
        EndPos = SearchPathLeft.find_first_of(pDelimiters);
    }                

    if (SearchPathLeft.size() && SearchPathLeft.at(SearchPathLeft.size() - 1) != '/') {
        SearchPathLeft += "/";
    }

    string FileWithPath = SearchPathLeft + sFilename;
    if (fileExists(FileWithPath)) {
        return FileWithPath;
    }

    if (fileExists(sFilename)) {
        return sFilename;
    }

    return "";
}

void readWholeFile(const string& sFilename, string& sContent)
{
    ifstream File(sFilename.c_str());
    if (!File) {
        throw Exception(AVG_ERR_FILEIO, "Opening "+sFilename+
                " for reading failed.");
    }
    vector<char> Buffer(65536);
    sContent.resize(0);
    while (File) {
        File.read(&(*Buffer.begin()), Buffer.size());
        sContent.append(&(*Buffer.begin()),File.gcount());
    }
    if (!File.eof() || File.bad()) {
        throw Exception(AVG_ERR_FILEIO, "Reading "+sFilename+
                " failed.");
    }
}

void writeWholeFile(const string& sFilename, const string& sContent)
{
    ofstream outFile(sFilename.c_str());
    if (!outFile) {
        throw Exception(AVG_ERR_FILEIO, "Opening "+sFilename+
                " for writing failed.");
    }
    outFile << sContent;
}

}
