#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <libconfig.h>
#include <stdio.h>

 #include <libxml/parser.h>
 #include <libxml/tree.h>


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

 int main(int argc, char **argv)
 {
    xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;

    if (argc != 2)  return(1);
    doc = xmlReadFile(argv[1], NULL, 0);
    
	std::vector<box> boxes;
    
    
    
    
    if (doc == NULL){
       printf("error: could not parse file %s\n", argv[1]);
       exit(-1);
       }

    
    root_element = xmlDocGetRootElement(doc);
    boxes=ReadXML(root_element);

		
	xmlFreeDoc(doc);       // free document
    xmlCleanupParser();    // Free globals
    	
    
	for (int i =0; i<boxes.size(); i++) {
		std::cout<< "time "<<boxes[i].time<<std::endl;
		std::cout<<"position_X "<< boxes[i].position_X<<std::endl;
		std::cout<< "position_Y " <<boxes[i].position_Y<<std::endl;
		std::cout<< "width "<<boxes[i].width<<std::endl;
		std::cout<< "height "<<boxes[i].height<<std::endl;
		std::cout<< "confidence "<<boxes[i].confidence<<std::endl;
		std::cout<< "text " <<boxes[i].text<<std::endl;
	}
    return 0;
 }
