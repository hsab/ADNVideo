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
            double position_X;
            double position_Y;
			double width;
			double height;
			double confidence;
            std::string text;

    };
    
    
        class OCR_track {
        public:
            double start;
            double end;
            double position_X;
            double position_Y;
			double width;
			double height;
			double confidence;
            std::string text;
    };
    
    
    
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
				xmlAttrPtr attr = cur_node->properties; 
				if (attr != NULL){
					   if ((!xmlStrcmp(xmlGetProp(cur_node, attr->name), (const xmlChar *)"Position_X")))  {std::string sName((char*) cur_node->children->content); b.position_X=::atof(sName.c_str());}
					   if ((!xmlStrcmp(xmlGetProp(cur_node, attr->name), (const xmlChar *)"Position_Y")))  {std::string sName((char*) cur_node->children->content); b.position_Y=::atof(sName.c_str());}
					   if ((!xmlStrcmp(xmlGetProp(cur_node, attr->name), (const xmlChar *)"time")))  {std::string sName((char*) cur_node->children->content); b.time=::atof(sName.c_str());}
					   if ((!xmlStrcmp(xmlGetProp(cur_node, attr->name), (const xmlChar *)"Width")))  {std::string sName((char*) cur_node->children->content); b.width=::atof(sName.c_str());}
					   if ((!xmlStrcmp(xmlGetProp(cur_node, attr->name), (const xmlChar *)"Height")))  {std::string sName((char*) cur_node->children->content); b.height=::atof(sName.c_str());}
					   if ((!xmlStrcmp(xmlGetProp(cur_node, attr->name), (const xmlChar *)"Confidence")))  {std::string sName((char*) cur_node->children->content); b.confidence=::atof(sName.c_str());}
					   if ((!xmlStrcmp(xmlGetProp(cur_node, attr->name), (const xmlChar *)"text")))  {
							std::string sName((char*) cur_node->children->content);
							b.text=sName; 							
							boxes.push_back(b);              
						}
						
				}
				cur_node = cur_node->next;
			}
				
		}	
		cur = cur->next;
    }
	return boxes;
 }



 
 int main(int argc, char **argv){
    
    if (argc != 2)  {
		std::cout<< "Usage "<< argv[0] <<" result.xml (OCR XML output file)" <<std::endl;
		return 0;
		}
   
  	xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
	doc = xmlReadFile(argv[1], NULL, 0);
    
	std::vector<box> boxes;
    std::vector<box> boxes_t;
    if (doc == NULL){
       printf("error: could not parse file %s\n", argv[1]);
       exit(-1);
    }

    
    root_element = xmlDocGetRootElement(doc); 
    boxes=ReadXML(root_element);
	xmlFreeDoc(doc);       // free document
    xmlCleanupParser();    // Free globals
    	
	
	boxes_t=boxes;
	box box_1=boxes[1];
	cv::Rect r1, r2, intersection;
	r1.x=box_1.position_X;
	r1.y=box_1.position_Y;
	r1.width=box_1.width;
	r1.height=box_1.height;
	float area_1, area_2, area_3;
	area_1= r1.width*r1.height;
	size_t lv_distance;
	bool change = true;
	
	
	for (int i =1; i<boxes_t.size(); i++) {
		r2.x=boxes_t[i].position_X;
		r2.y=boxes_t[i].position_Y;
		r2.width=boxes_t[i].width;
		r2.height=boxes_t[i].height;
		area_2= r2.width*r2.height;
		intersection= r1&r2;
		area_3= intersection.width*intersection.height;
		float recouvrement=area_3/(area_1 + area_2 - area_3);
		
		lv_distance= LevenshteinDistance(box_1.text,boxes_t[i].text);
		
		
		if ((recouvrement>0.90) & (lv_distance<10)){
			
			std::cout<< "lv "<<lv_distance<<std::endl;
			std::cout<< "time "<<boxes[i].time<<std::endl;
			std::cout<<"position_X "<< boxes[i].position_X<<std::endl;
			std::cout<< "position_Y " <<boxes[i].position_Y<<std::endl;
			std::cout<< "width "<<boxes[i].width<<std::endl;
			std::cout<< "height "<<boxes[i].height<<std::endl;
			std::cout<< "confidence "<<boxes[i].confidence<<std::endl;
			std::cout<< "text " <<boxes[i].text<<std::endl;
		}
		
	}

	
		std::string s0 = "ONSULTANT FOOTBALL RMC SPORT";
        std::string s1 = "CONSULTANT FOOTBALL RMC SPORT";
		std::cout << "distance between " << s0 << " and " << s1 << " : "  << LevenshteinDistance(s0,s1) << std::endl;
        return 0;
}
