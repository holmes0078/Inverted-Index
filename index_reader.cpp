//
//  index_reader.cpp
//  
//  queries the index for word and returns top 10 url's according to BM25 ranking
//
//  Created by Tushar Ahuja on 10/19/17.
//

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <math.h>
#include <boost/lexical_cast.hpp>
#include <algorithm>
#include <vector>

#define TOTAL_DOCUMENTS 8227947
#define AVG_LENGTH 852
#define K1 1.2
#define B 0.75

using namespace std;
class reader{
    
    std::fstream index_file;
    unsigned long int posting;
    short int posting_freq;
    std::unordered_map<unsigned long int, pair<std::string,int>> url_table;
    std::unordered_map<std::string, std::tuple<unsigned long int, unsigned long int, int>> lex_table;
    int temporary = 0;
    
    public:
        void load_stuff(){
            std::cout<<"Loading stuff ...."<<std::endl;
            clock_t begin = clock();
        
            std::fstream url_file("url_table.txt", std::ios::in);
            std::string str, url;
            unsigned long int doc_id;
            int mod_d;
            while(getline(url_file, str)){
                std::istringstream iss(str);
                iss >> doc_id;
                iss >> url;
                iss >> mod_d;
                url_table[doc_id] = make_pair(url,mod_d);
            }

            cout << "Loaded URL table" << endl;

            unsigned long int start_pos, end_pos;
            unsigned long int num_docs;
            int f_t_t;
            std::string line, word;
            std::fstream lex_file("lexicon_final.txt", std::ios::in);
            
            while(getline(lex_file, line)){
                std::istringstream iss(line);
                iss >> word;
                iss >> start_pos;
                iss >> end_pos;
                iss >> f_t_t;
                lex_table[word] = std::make_tuple(start_pos,end_pos, f_t_t);
            }
            
            lex_file.close();
            
            clock_t end = clock();
            
            std::cout << std::endl << "Time to load :: " <<(end-begin)/CLOCKS_PER_SEC << " Seconds"<<std::endl;
        }
    

    
    unsigned long int nextGEQ(vector<unsigned long int> & lp, int i, unsigned long int end_offset, unsigned long int k){
        //find first doc_id in inverted list that is >=k
        
        index_file.seekg(lp[i]);
        while(index_file.tellg() != end_offset){
            posting = 0;
            posting_freq = 0;
            index_file.read ((char*)&posting, sizeof(posting));
            if ((posting) >= k){
                lp[i] = index_file.tellg();
                return posting;
            }
            else{
                
                index_file.read ((char*)&posting_freq, sizeof(posting_freq));
            }
        }
        return -1;
    }

    short int getFreq(vector<unsigned long int> & lp, int i, unsigned long int end_offset, unsigned long int did){
        
        index_file.seekg(lp[i]);
        index_file.read((char*)&posting_freq, sizeof(posting_freq));
        lp[i] = index_file.tellg();
        return posting_freq;
    
    }

    double BM25(int fdt, int mod_d, int f_t){
        
        int big_k, iter=0;
        big_k = K1 * ((1-B) + (B * float(mod_d/AVG_LENGTH)));
        return log( ((TOTAL_DOCUMENTS-f_t+0.5)/(f_t+0.5)) ) * (((K1+1) * fdt)/(big_k + fdt)) ;
    }
    
    void query(string query){
        
        int dummy;
        vector<unsigned long int> doc_array;
        unsigned long int did, d = 0;
        vector<string> terms;
        double score = 0.0, score_two = 0.0;
        int num = 0;
        int i,j,k, disjunctive = 1;
        vector<pair<unsigned long int, double>> results;
        vector<pair<unsigned long int, double>> final_disjunctive_results;
        
        vector<unsigned long int> lp;
        vector<unsigned long int> lp_2;
        vector<unsigned long int> ep;
        vector<int> freq;
        istringstream ss(query);
        std::string token;
        int f_t, mod_d, iter=0;

        while(std::getline(ss, token, ' ')) {
            if (lex_table.count(token) != 0){
                terms.push_back(token);
                lp.push_back(get<0>(lex_table[token]));
                lp_2.push_back(get<0>(lex_table[token]));
                ep.push_back(get<1>(lex_table[token]));
                freq.push_back(get<2>(lex_table[token]));
                num += 1;
            }
            else{
                cout << endl << "Missing: " << token << endl;
            }
        }
        
        if (num > 0){
        did = 0;
        index_file.open("index_final.bin", std::ios::binary | std::ios::in);
        while (did <= TOTAL_DOCUMENTS)
        {
            /* get next post from shortest list */
            did = nextGEQ(lp, 0, ep[0], did);

            if (did == -1)
                break;
            
            for (i=1; (i<num); i++){
                
                d = nextGEQ(lp, i, ep[i], did);
                
                if (d != did){
                    break;
                }
            }
            
            if (d > did){
                did = d;
                /* not in intersection or disjunctive*/
                break;
            }
            else
            {
                disjunctive = 0;
                /* docID is in intersection; now get all frequencies */
                for (j=0; (j<num); j++){
                    dummy = getFreq(lp, j, ep[j], did);
                    /* compute BM25 score from frequencies and other data */
                    mod_d = get<1>(url_table[did]);
                    f_t = freq[j];
                    score += BM25(dummy, mod_d, f_t);
                }
                results.push_back(std::make_pair(did, score));
                score = 0.0;
                did++;
            }
        }
        
        if (disjunctive){
            cout << "Disjunctive Query" << endl;
            for (k=0; k<num; k++){
                posting = 0;
                posting_freq = 0;
                index_file.seekg(lp_2[k]);
                while(index_file.tellg() != ep[k]){
                        
                    index_file.read ((char*)&posting, sizeof(posting));
                    index_file.read ((char*)&posting_freq, sizeof(posting_freq));
                        
                    mod_d = get<1>(url_table[posting]);
                    f_t = freq[k];
                    score_two += BM25(posting_freq, mod_d, f_t);
                    results.push_back(std::make_pair(posting, score_two));
                }
                    score_two = 0.0;
            }
            std::nth_element(results.begin(), results.begin()+9, results.end(), [](const pair<unsigned long int, double> &lhs, const pair<unsigned long int, double> &rhs){
                return (lhs.second < rhs.second);
            });
            
            for(auto i: results){
                final_disjunctive_results.push_back(i);
                iter += 1;
                if (iter == 10){
                    break;
                }
            }
            iter = 0;
            
            sort(final_disjunctive_results.begin(), final_disjunctive_results.end(), []( const std::pair<unsigned long int, double> &lhs, const std::pair<unsigned long int, double> &rhs){
                return (lhs.second < rhs.second);
            });
            
            for(auto it = final_disjunctive_results.crbegin(); it != final_disjunctive_results.crend(); ++it){
                cout << get<0>(url_table[get<0>(*it)]) << " " << get<1>(*it) << endl;
                iter += 1;
                if (iter == 10){
                    break;
                }
            }
            index_file.close();
        }
        else{
            sort(results.begin(), results.end(), [ ]( const std::pair<unsigned long int, double> &lhs, const std::pair<unsigned long int, double> &rhs){
                             return (lhs.second < rhs.second);
            });
            
            for(auto it = results.crbegin(); it != results.crend(); ++it){
                cout << get<0>(url_table[get<0>(*it)]) << " " << get<1>(*it) << endl;
                iter += 1;
                if (iter == 10){
                    break;
                }
            }
        }
        index_file.close();
    }
    }
};

int main(){
    std::string word;
    
    reader r;
    r.load_stuff();
    while(1){
        std::cout << "Enter word ::";
        std::getline(std::cin, word);
        clock_t begin = clock();
        r.query(word);
        clock_t end = clock();
        std::cout << std::endl << "Time :: " <<(double)(end-begin)/CLOCKS_PER_SEC << " Seconds"<<std::endl;
    }
    return 0;
}





