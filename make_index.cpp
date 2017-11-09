//
//  make_index.cpp
//  
//
//  Created by Tushar Ahuja on 10/19/17.
//
#include <iostream>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <"varbyte_reader.cpp">

int main()
{
    std::streampos start_pos, end_pos;
    std::vector<std::string> post;
    std::ifstream sorted("sorted.txt", std::ios::in);
    std::fstream index_file;
    index_file.open("index.bin", std::ios::binary | std::ios::out);
    std::ofstream lex_file;
    lex_file.open("lexicon.txt", std::ios::out);
    std::string line;
    std::string word, prev_word;
    unsigned long int doc_id, prev_doc_id;
    prev_word = "";
    prev_doc_id = 0;
    int flag = 0;
    int line_num = 0;
    int num_docs = 0;
    short int freq;
    
    while(getline(sorted,line)){
        
        std::istringstream iss(line);
        iss >> word;
        iss >> doc_id;
        iss >> freq;
        
        if (word == prev_word){
//            std::cout << " " << doc_id;
            index_file.write ((char*)&doc_id, sizeof(doc_id));
            index_file.write ((char*)&freq, sizeof(freq));
            prev_word = word;
            num_docs += 1;
        }
        else{
            //can write num_docs for word
            //can write end position for prev_word
            //can write start position for word
            end_pos = index_file.tellg();
            if (!prev_word.empty()){
                lex_file << " " << end_pos << " " << num_docs <<std::endl;
            }
//            std::cout << " " << num_docs << std::endl;
            num_docs = 1;
//            std::cout << word << " " << doc_id;
            start_pos = index_file.tellg();
            lex_file << word << " " << start_pos;
            index_file.write ((char*)&doc_id, sizeof(doc_id));
            index_file.write ((char*)&freq, sizeof(freq));
            prev_word = word;
        }
    }
    index_file.close();
    lex_file.close();
}
