// Please read the wiki for information and build instructions.
#include <string>
#include <iostream>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <libconfig.h>
#include <stdio.h>
 

// accurate frame reading
#include "repere.h"
#include "binarize.h"
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>




class box {
        public:
            double time;
            int position_X;
            int position_Y;
			int width;
			int height;
			double confidence;
            std::string text;

};
    
    
class OCR_track {
        public:
            double start;
            double end;
            int position_X;
            int position_Y;
			int width;
			int height;
			double confidence;
            std::string text;
};
    
    
// Levenshtein distance of 2 strings
size_t LevenshteinDistance(const std::string &s1, const std::string &s2){		
		const size_t m(s1.size());
		const size_t n(s2.size());
		if( m==0 ) return n;
		if( n==0 ) return m;
		size_t *costs = new size_t[n + 1];
		for( size_t k=0; k<=n; k++ ) costs[k] = k;
		size_t i = 0;
		for (std::string::const_iterator it1 = s1.begin(); it1 != s1.end(); ++it1, ++i) {
			costs[0] = i+1;
			size_t corner = i;
			size_t j = 0;
			for ( std::string::const_iterator it2 = s2.begin(); it2 != s2.end(); ++it2, ++j ){
				size_t upper = costs[j+1];
				if( *it1 == *it2 ){
					costs[j+1] = corner;
				}
				else {
					size_t t(upper<corner?upper:corner);
					costs[j+1] = (costs[j]<t?costs[j]:t)+1;
				}
			corner = upper;
			}
		}
		size_t result = costs[n];
		delete [] costs;
		return result;
}

//read XML-file of the OCR results (tess-ocr-detector)
 std::vector<box>ReadXML(xmlNode * root_element) {
	std::vector<box> boxes;
	box b; 
    xmlNode *cur =NULL;
    xmlNode *cur_node =NULL;
    cur = root_element->xmlChildrenNode;
	while (cur != NULL)  {
		if (!xmlStrcmp(cur->name, (const xmlChar *)"box")){           
			cur_node=cur->xmlChildrenNode;
			while (cur_node != NULL){						
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"positionX"))  {std::string sName((char*) cur_node->children->content); b.position_X=::atof(sName.c_str());}
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"positionY"))  {std::string sName((char*) cur_node->children->content); b.position_Y=::atof(sName.c_str());}
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"time"))  {std::string sName((char*) cur_node->children->content); b.time=::atof(sName.c_str());}						
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"width"))  {std::string sName((char*) cur_node->children->content); b.width=::atof(sName.c_str());}
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"height"))  {std::string sName((char*) cur_node->children->content); b.height=::atof(sName.c_str());}
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"confidence"))  {std::string sName((char*) cur_node->children->content); b.confidence=::atof(sName.c_str());}
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"ocr") )  {
							if (cur_node->children != NULL){
								std::string sName((char*) cur_node->children->content);
								b.text=sName ; 
							}
							else {
								std::string sName = "XML_FAILED";
								b.text=sName; 
							}
							boxes.push_back(b);
						}
						cur_node = cur_node->next;
			}			   
		}	
		cur = cur->next;
    }
	return boxes;
}




std::vector<std::string>  Read_list(std::string file_name) {
	
	

	 std::vector<std::string> list;
    list.clear();
    
	std::fstream fichier(file_name.c_str());
    if ( !fichier ) {      
        std::cout << "fichier inexistant";
    } else {               
        bool continuer = true;
        while( continuer ) {	
            std::string ch;        
            fichier >> ch;	  
            if ( ch != "" ) {
				ch=ch  ;
                list.push_back(ch);
				
              }else           
                continuer = false; 
        }
    }
	
return list	;
}

int main(int argc, char **argv){

	// Usages
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --input                           specify the input XML file (OCR results)\n");
    options.AddUsage("  --list                             list of names \n");
    
	// if no option print the option-usage 
    if(options.Size() == 0)	options.Usage();
       
    std::string input = options.Get<std::string>("--input", "ocr.xml");
    std::string file_name = options.Get<std::string>("--list", "");
   	std::vector<std::string>  list_name;
   	list_name=Read_list(file_name);
   	

   	
	char *i_file = new char[input.length() + 1];
	strcpy(i_file, input.c_str());									
  	xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

	doc = xmlReadFile(i_file, NULL, 0);

    if (doc == NULL){
       std::cerr<<"Error: could not parse the XML file "<< input <<std::endl;
       exit(1);
    }
	delete [] i_file ;	

	std::vector<box> boxes_t;
    root_element = xmlDocGetRootElement(doc); 
	// read tess-ocr-detecor results
    boxes_t=ReadXML(root_element);
	
	// free document
	xmlFreeDoc(doc);       
    xmlCleanupParser(); 
    bool found =false;
    int i=0;
    std::string name="";
    std::string ocr;
    size_t t;
    while ((!found) & (boxes_t[i].time <200)){
		ocr=boxes_t[i].text;
		std::string buf;
		std::stringstream ss(ocr);
		std::vector<std::string> tokens;   
		while (ss>> buf){
			tokens.push_back(buf);
		}
		int k=0;
		while ((!found) & (k <tokens.size())){
			for (int j=0; j<list_name.size();j++) {		
				size_t L = LevenshteinDistance(list_name[j], tokens[k]);
				if (L<2){
					name=list_name[j];
					found=true;
					break;
				}
				
			}
			k++;
		}
		i++;
	}
	
	if (found) {
		
	   
        std::cout <<"anchor = "<< name <<std::endl;
	}
	return 0;
}
