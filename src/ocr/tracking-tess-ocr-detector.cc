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
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Position_X"))  {std::string sName((char*) cur_node->children->content); b.position_X=::atof(sName.c_str());}
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Position_Y"))  {std::string sName((char*) cur_node->children->content); b.position_Y=::atof(sName.c_str());}
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Time"))  {std::string sName((char*) cur_node->children->content); b.time=::atof(sName.c_str());			
				
							}						
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Width"))  {std::string sName((char*) cur_node->children->content); b.width=::atof(sName.c_str());}
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Height"))  {std::string sName((char*) cur_node->children->content); b.height=::atof(sName.c_str());}
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Confidence"))  {std::string sName((char*) cur_node->children->content); b.confidence=::atof(sName.c_str());}
						if (!xmlStrcmp(cur_node->name, (const xmlChar *)"Text"))  {std::string sName((char*) cur_node->children->content);
							b.text=sName; 
							boxes.push_back(b);
						}
						cur_node = cur_node->next;
			}			   
		}	
		cur = cur->next;
    }
	return boxes;
 }



 
int main(int argc, char **argv){
	
	// Usages
    amu::CommandLine options(argv, "[options]\n");
    options.AddUsage("  --input                           specify the input XML file (OCR results)\n");
    options.AddUsage("  --output                          specify the output XML file (Text boxes tracking)\n");
    
	// if no option print the option-usage 
    if(options.Size() == 0)	options.Usage();
       
    std::string output = options.Get<std::string>("--output", "ocr_filtred.xml");
    std::string input = options.Get<std::string>("--input", "ocr_results.xml");
   
	
	char *i_file = new char[input.length() + 1];
	strcpy(i_file, input.c_str());									
  	xmlDoc *doc = NULL;
    xmlNode *root_element = NULL;
	doc = xmlReadFile(i_file, NULL, 0);
    if (doc == NULL){
       std::cout<<"error: could not parse file "<< input <<std::endl;
       exit(1);
    }
	delete [] i_file ;	
    
	std::vector<box> boxes;
    std::vector<box> boxes_t;
    
    
    root_element = xmlDocGetRootElement(doc); 
    
    boxes=ReadXML(root_element);

	xmlFreeDoc(doc);       // free document
    xmlCleanupParser();    // Free globals    	
	boxes_t=boxes;
		
	std::vector<box> b;
	box box_1;
	std::vector<OCR_track> tracks;
	while (boxes_t.size()>1) {
					b.clear();
					box_1=boxes_t[0];
					boxes_t.erase (boxes_t.begin());
					b.push_back(box_1);
					OCR_track track;
					cv::Rect r1, r2, intersection;
					r1.x=box_1.position_X; r1.y=box_1.position_Y; r1.width=box_1.width;	r1.height=box_1.height;			
					float area_1, area_2, area_3;
					area_1= r1.width*r1.height;
					size_t lv_distance;
					bool ok = true;
					double recouvrement=0.0;
					int i=0;

					while (ok & i<boxes_t.size()){

						if 	(boxes_t[i].time != b[b.size()-1].time) {
							r2.x=boxes_t[i].position_X; r2.y=boxes_t[i].position_Y;	r2.width=boxes_t[i].width;	r2.height=boxes_t[i].height;	
							area_2= r2.width*r2.height;
							intersection= r1&r2;
							
							
							area_3= intersection.width*intersection.height;
							
							recouvrement=area_3/(area_1 + area_2 - area_3);
							lv_distance= LevenshteinDistance(b[b.size()-1].text,boxes_t[i].text);
							int t =std::abs(b[b.size()-1].text.size() - boxes_t[i].text.size());
							if ((recouvrement>0.2) &(lv_distance /boxes_t[i].text.size() <5) & (t<5)){
								b.push_back(boxes_t[i]);
								boxes_t.erase (boxes_t.begin()+i);	
								i--;
							}
						}
						i++;
						if ((boxes_t[i].time > b[b.size()-1].time +5) || i==boxes_t.size()-1) ok =false;
					}
					track.start=b[0].time;
					track.end=b[b.size()-1].time;
					track.confidence=0;
					track.text="TESSERCAT_FAILED";
					for (int j=0; j<b.size(); j++) {
						if (b[j].confidence > track.confidence){
							track.confidence=b[j].confidence;
							track.text=b[j].text;
							track.position_X=b[j].position_X;
							track.width=b[j].width;
							track.position_Y=b[j].position_Y;
							track.height=b[j].height;
							}
						}					
						if ((track.end - track.start >=0)& (track.text!= "TESSERCAT_FAILED") & ( track.text.size()>3)) {
							std::cout <<track.start << " " <<track.end <<" " <<track.position_X <<" " <<track.text <<std::endl; 
							tracks.push_back(track);
						}												
		}

		// write in the XML Format 
		xmlDocPtr doc_output = NULL;      
		xmlNodePtr root_node_output = NULL, node_output = NULL, box_node_output = NULL;
		doc_output = xmlNewDoc(BAD_CAST "1.0");
		root_node_output = xmlNewNode(NULL, BAD_CAST "root");
		xmlDocSetRootElement(doc_output, root_node_output);

    		for (int i=0;i<tracks.size();i++ ){
					box_node_output=xmlNewChild(root_node_output, NULL, BAD_CAST "box", BAD_CAST NULL);
					char buffer[100];
					//printf("%>2f %s\n",  b.time, sName.c_str());
					sprintf(buffer, "%.2f",tracks[i].start);
					node_output =  xmlNewChild(box_node_output, NULL, BAD_CAST "Start", (const xmlChar *) buffer);

					sprintf(buffer, "%.2f",tracks[i].end);
					node_output =  xmlNewChild(box_node_output, NULL, BAD_CAST "End", (const xmlChar *) buffer);
					
					sprintf(buffer, "%d",tracks[i].position_X);
					node_output =  xmlNewChild(box_node_output, NULL, BAD_CAST "Position_X", (const xmlChar *) buffer);
					
					sprintf(buffer, "%d",tracks[i].position_Y);	
					node_output =  xmlNewChild(box_node_output, NULL, BAD_CAST "Position_Y", (const xmlChar *) buffer);
								
					sprintf(buffer, "%d",tracks[i].width);
					node_output =  xmlNewChild(box_node_output, NULL, BAD_CAST "Width", (const xmlChar *) buffer);
										
					sprintf(buffer, "%d",tracks[i].height);
					node_output =  xmlNewChild(box_node_output, NULL, BAD_CAST "Height", (const xmlChar *) buffer);
										
					sprintf(buffer, "%.2f",tracks[i].confidence);
					node_output =  xmlNewChild(box_node_output, NULL, BAD_CAST "Confidence",(const xmlChar *) buffer);
					
					std::string str = tracks[i].text;
					char *cstr = new char[str.length() + 1];
					strcpy(cstr, str.c_str());
					node_output = xmlNewChild(box_node_output, NULL, BAD_CAST "Text",(const xmlChar *) cstr);
					delete [] cstr;	
				}
				
				

	
	char *o_file = new char[output.length() + 1];
	strcpy(o_file, output.c_str());														
	xmlSaveFormatFileEnc(o_file, doc_output, "UTF-8", 1);
	delete [] o_file ;	
							
	xmlFreeDoc(doc_output);
    xmlCleanupParser();
    xmlMemoryDump();

	return 0;
}
