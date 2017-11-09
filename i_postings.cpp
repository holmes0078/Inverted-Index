//
//  ipostings.cpp
//  Generates intermediate postings which are sorted using GNU sort command
//
//  Created by Tushar Ahuja on 10/10/17.
//

#include <iostream>
#include <fstream>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/filesystem.hpp>

struct path_leaf_string
{
    std::string operator()(const boost::filesystem::directory_entry& entry) const
    {
        return entry.path().leaf().string();
    }
};

void read_directory(const std::string& name, std::vector<std::string>& file_names)
{
    boost::filesystem::path p(name);
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    std::transform(start, end, std::back_inserter(file_names), path_leaf_string());
}


bool is_indexable(std::string token){

    if (token.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_") != std::string::npos)
    {
        return false;
    }
    else{
        return true;
    }
}

int main() {
    
    std::vector<std::string> file_names;
    
    read_directory("../data", file_names);
    
    
    unsigned int doc_id = 0;
    std::string url;
    std::string prev_url = "";
    
    bool check_url;
    bool check_words;
    
    std::vector<std::string> results;
    std::vector<std::string> words;
    std::map<int, std::pair<std::string, int>> url_table;
    std::set<std::pair<std::string, int>> postings;
    std::map<std::string, int> freq;
    std::string posting_name;
    int posting_num = 1;
    
    check_url = true;
    check_words = false;
    
    std::string file_name;
    
    std::ofstream url_file;
    url_file.open("url_table.txt", std::ios_base::app | std::ios_base::out);
    
    int temp = 1;
    
    for (auto i: file_names){
        if (i.find(".gz") != std::string::npos){
            file_name = "../data/" + i;
            
            posting_name = "posting" + std::to_string(posting_num) + ".txt";
            std::ofstream myfile;
            myfile.open(posting_name, std::ios_base::app | std::ios_base::out);
        
            std::ifstream file(file_name, std::ios_base::in | std::ios_base::binary);
            boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;
            inbuf.push(boost::iostreams::gzip_decompressor());
            inbuf.push(file);
            //Convert streambuf to istream
            std::istream instream(&inbuf);
            //Iterate lines
            std::string str;
            
            int d_size = 0;
            while(std::getline(instream, str)) {
                
                if ( (check_url) && (str.find("WARC-Target-URI") != std::string::npos) ) {
                    if (!prev_url.empty()){
                        url_table[doc_id-1] = std::make_pair(prev_url, d_size);
                    }
                    d_size = 0;
                    boost::split(results, str, boost::is_any_of(" "));
                    url = results[1];
                    boost::trim_right(url);
                    prev_url = url;
                    doc_id += 1;
                    check_url = false;
                    check_words = true;
                    
                    for (auto& i: postings){
                        myfile<< i.first << " " << i.second << " " << freq[i.first] << "\n";
                    }
                    postings.clear();
                    freq.clear();

                }
                
                if ((check_words) && (((str.find("Content-Length") == std::string::npos) && (str.find("WARC") == std::string::npos)) && (str.find("Content-Type") == std::string::npos))){
                    
                    boost::split(words, str, boost::is_any_of(" ,\t,.,|,:,--,-,(,),>,=,//,*,\",!"));
                    for(auto x : words){
                        
                        if (is_indexable(x))
                        {
                            if (!x.empty()){
                                boost::algorithm::to_lower(x);
                                if (freq.count(x) == 0){
                                    freq[x] = 1;
                                }
                                else{
                                    freq[x] += 1;
                                }
                                postings.insert(std::make_pair(x,doc_id));
                                d_size += 1;
                            }
                        }
                    }
                }
                else{
                    check_url = true;
                }
                
               If posting size greater than one million dump to file
               if (postings.size() >= 1000000){
               for (auto& i: postings){
                   myfile<< i.first << " " << i.second << "\n";
               }
               postings.clear();
               freq.clear();

               }
                
//                If url size greater than one million dump to file
                if(url_table.size() >= 1000000){
                    for (auto& i: url_table){
                        url_file << i.first << " " << std::get<0>(i.second) << " " << std::get<1>(i.second) << "\n";
                    }
                    url_table.clear();
                }
            }
            
            
            //Cleanup
            file.close();
            
            for (auto& i: url_table){
                url_file << i.first << " " << std::get<0>(i.second) << " " << std::get<1>(i.second) << "\n";
            }
            url_table.clear();
            
            for (auto& i: postings){
                myfile<< i.first << " " << i.second << "\n";
            }
            myfile.close();
            posting_num += 1;
            postings.clear();
        }
        
    }
    
    url_file.close();
    
    return 0;
}
