// bibo text file processor
// meant to be called as a script, so it does
// an action and then returns
// The list of text files is not allowed to change between runs,
// since we index them as an integer. The penalty for it being
// wrong is not severe, just the wrong character (or maybe no
// character).

// I haven't decided yet how the heck I'm going to do chat...
// by IP address would be simplest

// COMMANDS (argv[1]):
// list - dump an indexed list of selectable characters ready to process
// quote <n> - generate a random quote for character <n>
// scene <n1> <n2> - generate a random scene with two characters
// addchat <str> - add <str> to the active chat for this IP address

#include <string>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <vector>
using namespace std;
// up to three buffers - char 1, char 2, reply 1, reply 2
char* buf1, *buf2, *buf3, *buf4;
int len1, len2, len3, len4;
vector<string> nameList;    // only populated if we need it

// enable this to make the text go left/right instead of all stacked on the left
// it was hard to make that work right so I want to save the code ;)
//#define TEXT_LEFTRIGHT

// enable this to test for sizing against preferred background and
// reference character Cheerilee (specify char number)
// (comment out, not 0, to disable)
// OR, define on commandline: -DGFX_TEST=1
//#define GFX_TEST 1

// where is the cgi?
#define CHAT_URL "http://harmlesslion.com/ponychat/ponychat.cgi"

#ifdef _WIN32
#include <windows.h>
#define SRCPATH "D:\\work\\ponychat\\SeparateChars\\*.txt"
#define IMGPATH "D:\\work\\ponychat\\images\\*.png"
HANDLE hSrch;
WIN32_FIND_DATA findDat;

bool opendirect(string path, string /*ext*/) {
    hSrch = FindFirstFile(path.c_str(), &findDat);
    if (INVALID_HANDLE_VALUE == hSrch) return false;
    return true;
}

string getfilename() {
    return string(findDat.cFileName);
}

bool nextdir() {
    return FindNextFile(hSrch, &findDat);
}

void klosedir() {
    FindClose(hSrch);
}

FILE* filopen(const char* fn, const char* mode) {
    FILE* f;
    fopen_s(&f, fn, mode);
    return f;
}

string makefilename(string fn) {
    size_t p = fn.find('*');
    fn = fn.substr(0, p);
    fn += getfilename();
    return fn;
}


#else

// linux
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
DIR* dir;
struct dirent* d_ent;
string dirext;
#define SRCPATH "/home/ponychat/SeparateChars"
#define IMGPATH "/home/ponychat/ponyimages/"

void myreaddir() {
    if (NULL == dir) return;
    for (;;) {
        d_ent = readdir(dir);
        if (NULL == d_ent) return;
        string x = d_ent->d_name;
        if (x.length() <= 4) continue;
        if (x.substr(x.length() - 4, 4) != dirext) continue;
        break;
    }
}

bool opendirect(string path, string ext) {
    dirext = ext;
    dir = opendir(path.c_str());

    if (dir == NULL) return false;
    myreaddir();

    if (NULL == d_ent) return false;
    return true;
}

string getfilename() {
    if (NULL == d_ent) return "";
    return string(d_ent->d_name);
}

bool nextdir() {
    myreaddir();
    if (NULL == d_ent) return false;
    return true;
}

void klosedir() {
    closedir(dir);
}

#define filopen fopen

string makefilename(string fn) {
    fn += '/';
    fn += getfilename();
    return fn;
}

#endif

// case insensitive and whitespace agnostic string compare
const char* strtest(const char* a, const std::string &w) {
    if (a == NULL) return NULL;
    const char *b = w.c_str();
    int cnt = w.length() + 1;
    while (*a) {
        const char* p1 = a;
        const char* p2 = b;
        for (;;) {
            if ((*p1 == 0) || (*p2 == 0)) break;
            if ((*p2 <= '!') && (*p1 <= '!')) { ++p1; ++p2; continue; }
            if (toupper(*p1) == toupper(*p2)) { ++p1; ++p2; continue; }
            break;
        }
        if (*p2 == '\0') {
            return a;
        }
        ++a;
        --cnt;
        if (cnt == 0) break;
    }
    return NULL;
}
// reverse version - needs the base to know where to stop
const char* strrsearch(const char *base, const char* a, const char* b) {
    if ((a == NULL) || (b == NULL) || (base == NULL)) return NULL;
    while (a >= base) {
        const char* p1 = a;
        const char* p2 = b;
        for (;;) {
            if ((*p1 == 0) || (*p2 == 0)) break;
            if ((*p2 <= '!') && (*p1 <= '!')) { ++p1; ++p2; continue; }
            if (toupper(*p1) == toupper(*p2)) { ++p1; ++p2; continue; }
            break;
        }
        if (*p2 == '\0') {
            return a;
        }
        --a;
    }
    return NULL;
}

// add the style tags needed for the pic
void addstyle() {
    printf("<style>\n.parent{\n  position: relative;\n  top:0;\n  left:0;\n  right:100%%\n  z-index:0;\n}\n");
    printf(".over-img1{\n  position:absolute;\n  z-index:1;\n}\n");
    printf(".over-img2{\n  position:absolute;\n  z-index:2;\n  transform: scaleX(-1);\n}\n");
    printf(".over-img3{\n  position:absolute;\n  z-index:1;\n}\n");
    printf(".talkpad{  font-size:25px; display: inline-block; margin: 1px 0 1px 0; }\n");
#ifdef TEXT_LEFTRIGHT
    printf(".talk1{  font-size:25px; border: 1px solid black; border-radius:6px; background:#F0FFF0; display: inline-block; margin: 1px 25%% 1px 0; }\n");
    printf(".talk2{  font-size:25px; border: 1px solid black; border-radius:6px; background:#F0F0FF; display: inline-block; float:right; margin: 1px 0 1px 25%%; }\n");
#else
    printf(".talk1{  font-size:25px; border: 1px solid #d0dfd0; border-radius:6px; background:#F0FFF0; display: inline-block; margin: 1px 0 1px 0; }\n");
    printf(".talk2{  font-size:25px; border: 1px solid #d0d0df; border-radius:6px; background:#F0F0FF; display: inline-block; margin: 1px 0 1px 0; }\n");
#endif

    printf("</style>\n");
}

// extract size tags from a filename
void getsizes(const string &name1, int &width, int &voff) {
    width = 25;  // 25% by default
    voff = 5;    // 5% by default

    size_t p = name1.find("~~");
    if (p != string::npos) {
        p += 2;
        width = atoi(name1.c_str() + p);
        p = name1.find('~', p);
        if (p != string::npos) {
            ++p;
            voff = atoi(name1.c_str() + p);
        }
    }
}

// generate the bottom picture...
void makepic(const string &fn1, const string &fn2) {
    int numpics = 0;
    string bgname, name1, name2;
    // work out how many pics there are...
    if (!opendirect(IMGPATH, ".png")) return;
    for (;;) {
        if ((getfilename().substr(0,2) == "bg") && (isdigit(getfilename()[2]))) ++numpics;
        if (!nextdir()) break;
    }
    klosedir();

    printf("<!-- %d bgs -->\n", numpics);

    // now do it for real
    if (!opendirect(IMGPATH, ".png")) return;
    printf("<div class=\"parent\">\n");

    int img = rand() % numpics + 1;
#ifdef GFX_TEST
    img = 3; // preferred background - throneroom
#endif
    char buf[1024];
    sprintf(buf, "bg%d_by_", img);
    printf("<!-- %s -->\n", buf);

    for (;;) {
        if (getfilename().substr(0, strlen(buf)) == buf) {
            // got it!
            bgname = getfilename();
            printf("<img style=\"border: 1px solid black;\" width=\"100%%\" src=\"/ponyimages/%s\">\n", bgname.c_str());
            break;
        }
        if (!nextdir()) break;
    }
    klosedir();

    string clss;
    int offsetSize;  // used to calculate centering
    if (fn2.empty()) {
        clss = "over-img3";
        offsetSize = 100; // entire width
    } else {
        clss = "over-img1";
        offsetSize = 50;  // just half
    }

    strncpy(buf, fn1.c_str(), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    char *p = strrchr(buf, '.');
    if (p) *p = '\0';
    p = strrchr(buf, '/');
    if (p) memmove(buf, p + 1, strlen(p));
    p = strrchr(buf, '\\');
    if (p) memmove(buf, p + 1, strlen(p));
    strcat(buf, "_by_");
    printf("<!-- %s -->\n", buf);

    if (opendirect(IMGPATH, ".png")) {
        for (;;) {
            if (getfilename().substr(0, strlen(buf)) == buf) {
                // got it
                name1 = getfilename();

                // filename can specify alternate width and vertical offset
                int width, voff, hoff;
                getsizes(name1, width, voff);
                if (width < offsetSize) {
                    hoff = (offsetSize - width) / 2;
                } else {
                    hoff = 1;
                }
                printf("<img width=\"%d%%\" src=\"/ponyimages/%s\" class=\"%s\" style=\"bottom:%d%%; left:%d%%;\">\n", width, name1.c_str(), clss.c_str(), voff, hoff);
                break;
            }
            if (!nextdir()) break;
        }
        klosedir();
    }

    if (!fn2.empty()) {
        clss = "over-img2";

        strncpy(buf, fn2.c_str(), sizeof(buf));
        buf[sizeof(buf) - 1] = '\0';
        char *p = strrchr(buf, '.');
        if (p) *p = '\0';
        p = strrchr(buf, '/');
        if (p) memmove(buf, p + 1, strlen(p));
        p = strrchr(buf, '\\');
        if (p) memmove(buf, p + 1, strlen(p));
        strcat(buf, "_by_");
        printf("<!-- %s -->\n", buf);

        if (opendirect(IMGPATH, ".png")) {
            for (;;) {
                if (getfilename().substr(0, strlen(buf)) == buf) {
                    // got it
                    name2 = getfilename();
                    // filename can specify alternate width and vertical offset
                    int width, voff, hoff;
                    getsizes(name2, width, voff);
                    if (width < offsetSize) {
                        hoff = (offsetSize - width) / 2;
                    } else {
                        hoff = 1;
                    }
                    printf("<img width=\"%d%%\" src=\"/ponyimages/%s\" class=\"%s\" style=\"bottom:%d%%; right:%d%%;\">\n", width, name2.c_str(), clss.c_str(), voff, hoff);
                    break;
                }
                if (!nextdir()) break;
            }
            klosedir();
        }
    }
    printf("</div>\n");

    printf("\n<font size=\"-1\">\n");
    printf("%s<br>\n", bgname.c_str());
    printf("%s<br>\n", name1.c_str());
    printf("%s<br>\n", name2.c_str());
    printf("</font>\n");
}

// pull the current word and return it, update pos
string pullword(char* buf, int len, int& pos) {
    while ((pos < len) && (buf[pos] == ' ')) ++pos;
    if (pos >= len) {
        // end of file
        return string("");
    }
    if (buf[pos] < ' ') {
        // end of line
        return string("");
    }
    // return till next whitespace
    string x;
    while (buf[pos] > ' ') {
        x += buf[pos++];
    }
    return x;
}

// create one random sentence from the filename
string generateLine(char *buf1, int len1, char *buf2, int len2) {
    string output;
    static char* buf = NULL;
    static int buflen = 0;
    int len;

    if ((buf2 == NULL)||(len2==0)) {
        if (len1 > buflen) {
            buf = (char*)realloc(buf, len1 + 1);
            buf[len1] = '\0';
            buflen = len1;
        }
        memcpy(buf, buf1, len1);
        buf[len1] = 0;
        len = len1;
    } else {
        // we want to kind of balance out the chat log with the source text...
        // 50:50 was cute but repetitive, 75:25 a bit sparse, try 65:35
        int l2cnt = (len1 * 35 / 100) / len2;
        if (l2cnt == 0) l2cnt = 1;
        if (len1 + len2*l2cnt > buflen) {
            buf = (char*)realloc(buf, len1 + len2*l2cnt + 1);
            buf[len1 + len2*l2cnt] = '\0';
            buflen = len1 + len2*l2cnt;
        }
        memcpy(buf, buf1, len1);
        int p = len1;
        for (int idx = 0; idx < l2cnt; ++idx) {
            memcpy(&buf[p], buf2, len2);
            p += len2;
            buf[p] = 0;
        }
        len = len1 + len2 * l2cnt;
    }

    int pos = rand() % len1;  // force the new string to start in the char's voice
    if (pos > 0) {
        // seek to the beginning of a line
        while (pos < len) {
            if (buf[pos - 1] == '\n') break;
            ++pos;
        }
        if (pos >= len) pos = 0;
    }

    // pull 'x' words, ending if we reach end of line.
    string w;
    for (;;) {
        int cnt = rand() % 5 + 1;
        for (int idx = 0; idx < cnt; ++idx) {
            w = pullword(buf, len, pos);
            if (w.empty()) break;
            if ((w[w.length() - 1] == ']') && (w[0] != '[')) {
                // remove close braces without an open
                int p = (int)output.length() - 2; // -2 to skip the bracket we found
                while (p >= 0) {
                    if (output[p] == '[') break;
                    if (output[p] == ']') p = 0;
                    --p;
                }
                if (p < 0) {
                    output += w.substr(0, w.length() - 1) + ' ';
                    continue;
                }
            }
            output += w + ' ';
        }

        // if we have no word, then exit
        if (w.empty()) break;
        // if we have punctuation (except comma or apostrophe), then we ended on an end word
        //if ((w[w.length() - 1] < 'A') && (w[w.length() - 1] != ',') && (w[w.length() - 1] != '\'')) break;
        // instead, explicitly check for . (not ...), !, ?
        char c = '\0';
        if (w.length() > 1) c=w[w.length()-1];
        if ((c == '.') && (w[w.length()-2] != '.')) break;
        if (c == '?') break;
        if (c == '!') break;

        // find a new instance of this same word
        // by searching backwards, we reduce repetition in sentences that contain it
        // more than once by finding the LAST instance first.
        // fixes "She won't admit it, but she won't admit it, but she won't admit it, but she doesn't like it"
        pos = rand() % len;
        w = ' ' + w + ' ';
        const char* p = strrsearch(buf, &buf[pos], w.c_str());
        if (NULL == p) {
            // try from the end
            int end = strlen(buf);
            p = strrsearch(&buf[pos], &buf[end], w.c_str());
            if (NULL == p) {
                // the only case this SHOULD be caused by is first word in the file,
                // so try that directly
                w = w.substr(1);
                if (buf == strtest(buf, w)) {
                    p = buf;
                } else {
                    output += "then I lost my place! ";
                    goto finish;
                }
            }
        }

        // skip to the end of the word, then loop
        pos = (int)(p - buf) + (int)w.length();
    }

finish:
    // look for open brace with no close, and add one (asides, etc)
    bool brace = false;
    for (unsigned int idx = 0; idx < output.length(); ++idx) {
        if (output[idx] == '[') {
            if (brace) {
                // nested brace, fix it
                output.replace(idx, 1, "][");
                brace = false;
            } else {
                brace = true;
            }
        } else if (output[idx] == ']') {
            brace = false;
        }
    }
    if (brace) {
        output[output.length() - 1] = ']';
        output += ' ';
    }

    // make sure we got something - blank lines in the database give us empty strings
    // (which the caller will deal with)
    if (output.length() >= 2) {
        // capitalize first letter
        output[0] = toupper(output[0]);

        // if no punctuation at end, add one (space also at end)
        if (NULL == strchr(".!?]", output[output.length() - 2])) {
            output[output.length() - 1] = '.';
            output += ' ';
        }
    }
    return output;
}

// return a valid random filename
int randomfile() {
    if (!opendirect(SRCPATH, ".txt")) {
        printf("no dir\n");
        return 0;
    }

    int cnt = 0;
    for (;;) {
        ++cnt;
        if (!nextdir()) {
            klosedir();
            return rand() % cnt + 1;
        }
    }

    return 0;
}

// single string replace
void strreplace(string& s, string src, string rep) {
    for (;;) {
        size_t p = s.find(src);
        if (string::npos == p) break;
        s.replace(p, src.length(), rep);
    }
}

// this is a tricky one - you can become me or I
// to be cheap and cheesy, if it's in the first half of the
// string, we'll use I, else me
// We go a little further and split the sentence at a single
// and or but, which helps in those cases.
void strreplaceyou(string& s, string src, string rep1, string rep2) {
    size_t conjunct = s.find(" and ");
    if (conjunct == string::npos) conjunct = s.find(" but ");
    if (conjunct == string::npos) conjunct = s.find(" if ");
    if (conjunct == string::npos) conjunct = s.find(" with ");
    if (conjunct == string::npos) conjunct = s.find(" how ");

    for (;;) {
        size_t p = s.find(src);
        if (string::npos == p) break;
        string replace = rep1;
        if (conjunct == string::npos) {
            if (p >= s.length() / 2) replace = rep2;
        } else {
            if (p < conjunct) {
                if (p >= conjunct / 2) replace = rep2;
            } else {
                if (p >= conjunct + (conjunct / 2)) replace = rep2;
            }
        }
        s.replace(p, src.length(), replace);
    }
    
}


// swap pronouns
void fixpronouns(string& s) {
    // inverts pronouns to make talking back make a little more sense
    if (s[0] != 'I') s[0] = tolower(s[0]);
    s = ' ' + s + ' ';

    // substitute with temp strings so the second pass is clean
    strreplace(s, " am ", " `are ");
    strreplace(s, " was ", " `were ");
    strreplace(s, " wasn't ", " `weren't ");
    strreplace(s, " my ", " `your ");
    strreplace(s, " you've ", " `I've ");
    strreplace(s, " you're ", " `I'm ");
    strreplaceyou(s, " you ", " `I ", " `me ");

    // second pass
    strreplace(s, " are ", " am ");
    strreplace(s, " were ", " was ");
    strreplace(s, " weren't ", " wasn't ");
    strreplace(s, " your ", " my ");
    strreplace(s, " I've ", " you've ");
    strreplace(s, " I'm ", " you're ");
    strreplace(s, " me ", " you ");
    strreplace(s, " us ", " you ");
    strreplace(s, " we ", " you ");
    strreplace(s, " I ", " you ");

    // fix up first pass
    strreplace(s, " `are ", " are ");
    strreplace(s, " `were ", " were ");
    strreplace(s, " `weren't ", " weren't ");
    strreplace(s, " `your ", " your ");
    strreplace(s, " `I've ", " I've ");
    strreplace(s, " `I'm ", " I'm ");
    strreplace(s, " `I ", " I ");
    strreplace(s, " `me ", " me ");

    // special peephole optimizations...
    strreplace(s, " you want I ", " you want me ");
    strreplace(s, " Well, ", " Wait, ");

    s = s.substr(1, s.length() - 2);
    s[0] = toupper(s[0]);
}

// convert a filename into a character name
string parseToName(const string &fn) {
    string un1;
    for (unsigned int idx = 0; idx < fn.length(); ++idx) {
        if (fn[idx] == '.') break;
        // >1 covers AK Yearling and gives room to test for Mc
        if ((idx > 1) && (fn[idx] >= 'A') && (fn[idx] <= 'Z') 
            // special case for Hoo'Far
            && (fn[idx-1] != '\'')
            // special case for McColt and McIntosh
            && ((fn[idx-2] != 'M')||(fn[idx-1] != 'c'))
            ) 
        {
            un1 += ' ';
        }
        un1 += fn[idx];
    }
    return un1;
}

// fills in nameList if needed
void populateNameList() {
    if (nameList.size() > 0) return;    // already loaded

    if (!opendirect(SRCPATH, ".txt")) {
        printf("No dir for namelist\n");
        return;
    }

    do {
        nameList.emplace_back(parseToName(getfilename()));
    } while (nextdir());

    klosedir();

// a couple of honorable mentions... this is MLP specific
    nameList.emplace_back("Sonata Blaze");
    nameList.emplace_back("Gummy");
    nameList.emplace_back("Impossibly Rich");
    nameList.emplace_back("Princess Twilight");
    nameList.emplace_back("Scoot");
    nameList.emplace_back("Pink Pony");
    nameList.emplace_back("Big Mac");
    nameList.emplace_back("Big MacIntosh");
    nameList.emplace_back("Sunny Skies");
    nameList.emplace_back("Pear Butter");
    nameList.emplace_back("Star Swirl");
    nameList.emplace_back("Pumkpin Cake");
    nameList.emplace_back("Pound Cake");
    nameList.emplace_back("Rain Shine");
    nameList.emplace_back("Ms Powerful");
    nameList.emplace_back("Fern Flare");
    nameList.emplace_back("Rare");
    nameList.emplace_back("Rara");
    nameList.emplace_back("Bon Bon");
    nameList.emplace_back("Rainbooms");
    nameList.emplace_back("Lofty");

    // debug
//    for (string x : nameList) {
//        printf("<!-- '%s' -->\n", x.c_str());
//    }
}

// replace a potential name tstname in str with on at p
// return true if we did it
bool replaceName(const string &tstname, string &str, const string &n, size_t p, bool isFirst) {
    // If we get here, then it's a match! Replace with the other character
    // First, we need to choose a word from the name
    string on = n;
    // strip down to first or last name, except rarely
    if (rand()%100 > 15) {
        size_t op = on.find(' ');
        if (op != string::npos) {
            // choose a word
            if (rand()%100 > 65) {
                // second word is less respectful
                on = on.substr(op+1);
            } else {
                on = on.substr(0, op);
            }
        }
    }

    // fix up certain pronouns
    if (on == "Mr") on = "Sir";
    else if (on == "Miss") on = "Ma'am";
    else if (on == "Mrs") on = "Ma'am";
    else if (on == "Ms") on = "Ma'am";
    else if (on == "Dr") on = "Doctor";
    else if (on == "Big") on = n;       // just "Big" doesn't make sense (Big MacIntosh, Big Daddy McColt)
    else if (on == "Grand") on = n;     // just "Grand" doesn't make sense (Grand Pear)
    else if (on == "Iron") on = n;      // just "Iron" doesn't make sense (Iron Will)

    // and do the replace
    if (isFirst) {
        // first word, but there may be some before
        string first;
        if (p > 0) {
            // if we are in the middle, we MUST be followed by a comma
            if (str[p+tstname.length()] != ',') return false;
            first = str.substr(0,p);
        }
        str = first + on + str.substr(p+tstname.length());
    
        printf("<!-- replace '%s' with '%s' (first:%d) -->\n", tstname.c_str(), on.c_str(), isFirst);
    } else {
        // last word plus punctuation
        // might be an elipsis
        string last;
        size_t pp = str.length()-1;
        while (!isalpha(str[pp])) {
            --pp;
        }
        if (pp < str.length()) ++pp;
        last = str.substr(pp);
        str = str.substr(0, pp);

        str = str.substr(0, p) + on + last;

        printf("<!-- replace '%s' with '%s' (first:%d) -->\n", tstname.c_str(), on.c_str(), isFirst);
    }

    return true;
}

// perform name substitution in the string str, us is our name
// n is the name of the other character, if we decide to use it
void nameSubstitution(string &str, const string &n, const string &us) {
    // Rules: It might be a name to substitute only if it's the
    // first or the name. Punctuation is used to verify. (If it's
    // in the middle, it's more likely a third party).
    string tstname;

    // get the names from the disk
    populateNameList();

    // make sure beginning and end have no spaces
    while ((str.length())&&(str[0] == ' ')) str = str.substr(1);
    while ((str.length())&&(str[str.length()-1] == ' ')) str=str.substr(0,str.length()-1);

    // go through the name list, and replace only first or last words in the sentence,
    // with separating punctuation
    // First pass we search ONLY complete names, second pass we split it up. This
    // helps prevent ordering issues (for instance "Diamond" will match for "Diamond Tiara"
    // before "Double Diamond", even if "Double Diamond" is what was in the text)
    for (int pass = 0; pass < 2; ++pass) {
        for (string x : nameList) {
	        if (x == us) continue;  // don't replace self references

            size_t p = string::npos;
            size_t l = 0;
            string tstname;

            if (pass == 0) {
                p = str.find(x);
                if (p != string::npos) {
                    tstname = x;
                    l = x.length();
                }
            } else {
                // try some variations
                if (string::npos != x.find(' ')) {
                    // (note there may be more than two names!)
                    string n1,n2;
                    n1 = x.substr(0, x.find(' '));
                    // special case 'Big' and 'Dragon'
                    if (n1 == "Big") {
                        n1 = x.substr(0, x.find(' ', 4));
                    }
                    if (n1 == "Dragon") {
                        n1 = x.substr(0, x.find(' ', 7));
                    }
                    n2 = x.substr(x.find(' ',n1.length())+1);
                    // extra special case for 'The'
                    if (n1.substr(0,4) == "The ") n1=n1.substr(4);

                    if (n1.length() != x.length()) {
                        // replace "Princess" with "Principal" (Celestia and Luna)
                        if (n1 == "Princess") {
                            tstname = "Principal " + n2;
                            p = str.find(tstname);
                            l = tstname.length();
                        }

                        // try Ms/Mrs/Mr lastname
                        if (p == string::npos) {
                            tstname = "Ms " + n2;
                            p = str.find(tstname);
                            l = tstname.length();
                        }
                        if (p == string::npos) {
                            tstname = "Mrs " + n2;
                            p = str.find(tstname);
                            l = tstname.length();
                        }
                        if (p == string::npos) {
                            tstname = "Miss " + n2;
                            p = str.find(tstname);
                            l = tstname.length();
                        }
                        if (p == string::npos) {
                            tstname = "Mr " + n2;
                            p = str.find(tstname);
                            l = tstname.length();
                        }

                        // first name
                        if (p == string::npos) {
                            tstname = n1;
                            p = str.find(n1);
                            l = n1.length();
                        }

                        // last name 
                        if (p == string::npos) {
                            tstname = n2;
                            p = str.find(n2);
                            l = n2.length();
                        }
                    }
                }
            }

            // if still nothing, search on
            if (p == string::npos) continue;

            // there is a match, check start or end, and full name! (Cause 'Ma' is short)
            if (strchr("!?,. ", str[p+tstname.length()])) {
                if ((p > 1) && (str[p-1]==' ') && (str[p-2] == ',')) {
                    // end
                    replaceName(tstname, str, n, p, false);
                    return;
                } else if ((strchr("!?,.", str[p+l])) && ((p==0)||(str[p-1]==' '))) {
                    // beginning
                    replaceName(tstname, str, n, p, true);
                    return;
                }
            }
        }
    }
}

// list (no arg)
void runlist() {
    if (!opendirect(SRCPATH, ".txt")) {
        printf("no dir\n");
        return;
    }

    printf("<html><body>");
    addstyle();

    int cnt = 1;
    for (;;) {
        printf("\n<a href=\"%s?%d\">%s</a>   \n", CHAT_URL, cnt++, getfilename().c_str());
        if (!nextdir()) {
            klosedir();
            printf("</body></html>");
            return;
        }
    }
}

// fix blank lines in database
void fixbuf(char *buf, int &len) {
    // initial blank lines
    while (buf[0] == '\n') {
        memmove(&buf[0], &buf[1], len--);
    }
    // internal blank lines
    char *p;
    while ((p = strstr(buf, "\n\n")) != NULL) {
        memmove(p, p+1, len-(p-buf));
        --len;
    }
}

// quote <char number>
void runquote(const char* who) {
    int w = atoi(who);
    if (w == 0) {
        w = randomfile();
    }
    if (!opendirect(SRCPATH, ".txt")) {
        printf("No dir\n");
    }
    printf("\n<html><body>\n");
    addstyle();

    while (--w > 0) {
        nextdir();
    }

    // print out the name of the character, from the filename
    std::string fn = getfilename();
    for (unsigned int idx = 0; idx < fn.length(); ++idx) {
        if (fn[idx] == '.') break;
        if ((idx > 0) && (fn[idx] >= 'A') && (fn[idx] <= 'Z')) printf(" ");
        printf("%c", fn[idx]);
    }
    printf(": ");

    // suck the file into memory
    fn = makefilename(SRCPATH);
    klosedir();

    FILE* fp = filopen(fn.c_str(), "r");
    if (NULL == fp) {
        printf("No file %s\n", fn.c_str());
        return;
    }
    fseek(fp, 0, SEEK_END);
    len1 = ftell(fp) + 1;
    fseek(fp, 0, SEEK_SET);
    buf1 = (char*)malloc(len1 + 2);
    buf1[0] = '\n';
    len1 = (int)fread(buf1 + 1, 1, len1, fp);
    fclose(fp);
    buf1[len1] = '\n';
    buf1[len1 + 1] = '\0';
    ++len1;
    // special case for databases with a blank lines
    fixbuf(buf1, len1);

    // now start babbling
    int cnt = rand() % 5 + 2;
    for (int idx = 0; idx < cnt; ++idx) {
        string s = generateLine(buf1, len1, NULL, 0);
        printf("%s", s.c_str());
        if (s.empty()) --idx;   // if there's a blank line in the database, then we get an empty output. Ignore it.
    }

    // and finally, generate the bottom image
    makepic(fn, "");

    printf("\n</body></html>\n");

}

// scene <char1> <char2>
void runscene(const char* who1, const char* who2) {
    int w = atoi(who1);
    if (w == 0) {
        w = randomfile();
    }
#ifdef GFX_TEST
    w = 81; // preferred reference is Cheerilee (check for changes)
#endif
    //printf("<!-- %d -->\n", w);

    if (!opendirect(SRCPATH, ".txt")) {
        printf("No dir\n");
    }

    int wold = w;
    while (--w > 0) {
        nextdir();
    }

    // work out the name of the character, from the filename
    std::string fn = getfilename();
    string un1 = parseToName(fn);

    // suck the file into memory
    fn = makefilename(SRCPATH);
    klosedir();

    FILE* fp = filopen(fn.c_str(), "r");
    if (NULL == fp) {
        printf("No file %s\n", fn.c_str());
        return;
    }
    fseek(fp, 0, SEEK_END);
    len1 = ftell(fp) + 1;
    fseek(fp, 0, SEEK_SET);
    buf1 = (char*)malloc(len1 + 2);
    buf1[0] = '\n';
    len1 = (int)fread(buf1 + 1, 1, len1, fp);
    fclose(fp);
    buf1[len1] = '\n';
    buf1[len1 + 1] = '\0';
    ++len1;
    // special case for databases with a blank lines
    fixbuf(buf1, len1);

    // now babbler 2
    w = atoi(who2);
    if (w == 0) {
        w = wold;
        while (wold == w) {
            w = randomfile();
        }
    }
#ifdef GFX_TEST
    w = GFX_TEST;
#endif
    //printf("<!-- %d -->\n", w);

    if (!opendirect(SRCPATH, ".txt")) {
        printf("No dir\n");
    }

    while (--w > 0) {
        nextdir();
    }

    // save the name
    string fn1 = fn;

    // work out the name of the character, from the filename
    fn = getfilename();
    string un2 = parseToName(fn);

    // suck the file into memory
    fn = makefilename(SRCPATH);
    klosedir();

    fp = filopen(fn.c_str(), "r");
    if (NULL == fp) {
        printf("No file %s\n", fn.c_str());
        return;
    }
    fseek(fp, 0, SEEK_END);
    len2 = ftell(fp) + 1;
    fseek(fp, 0, SEEK_SET);
    buf2 = (char*)malloc(len2 + 2);
    buf2[0] = '\n';
    len2 = (int)fread(buf2 + 1, 1, len2, fp);
    fclose(fp);
    buf2[len2] = '\n';
    buf2[len2 + 1] = '\0';
    ++len2;
    // special case for databases with a blank lines
    fixbuf(buf2, len2);

    printf("\n<html><body>\n");
    addstyle();

    // now start babbling
    len3 = 0;
    len4 = 0;
    int lps = rand() % 4 + 2;
    for (int lp = 0; lp < lps; ++lp) {
        int cnt = rand() % 1 + 1;
        if (lp & 1) {
#ifdef TALK_LEFTRIGHT
            printf("<p class=\"talkpad\">&nbsp</p>");
#endif
            printf("<p class=\"talk2\">\n");
            printf("<b>%s: </b>", un2.c_str());
            for (int idx = 0; idx < cnt; ++idx) {
                string s = generateLine(buf2, len2, buf4, len4);
                if (s.empty()) {
                    // if there's a blank line in the database, then we get an empty output. Ignore it.
                    --idx;
                    continue;
                }
                nameSubstitution(s, un1, un2);
                printf("%s", s.c_str());
                s += '\n';
                // add the string to the chat
                fixpronouns(s);
                buf3 = (char*)realloc(buf3, len3 + 1 + s.length());
                buf3[len3] = 0;
                if (NULL == buf3) {
                    printf("realloc failed\n");
                    return;
                }
                strcat(&buf3[len3], s.c_str());
                len3 += (int)s.length();
            }
            printf("</p><br>\n");
        } else {
            printf("<p class=\"talk1\">\n");
            printf("<b>%s: </b>", un1.c_str());
            for (int idx = 0; idx < cnt; ++idx) {
                string s = generateLine(buf1, len1, buf3, len3);
                if (s.empty()) {
                    // if there's a blank line in the database, then we get an empty output. Ignore it.
                    --idx;
                    continue;
                }
                nameSubstitution(s, un2, un1);
                printf("%s", s.c_str());
                s += '\n';
                // add the string to the chat
                fixpronouns(s);
                buf4 = (char*)realloc(buf4, len4 + 1 + s.length());
                buf4[len4] = 0;
                if (NULL == buf4) {
                    printf("realloc failed\n");
                    return;
                }
                strcat(&buf4[len4], s.c_str());
                len4 += (int)s.length();
            }
            printf("</p><br>\n");
        }
    }

    // bottom image
#ifndef TEXT_LEFTRIGHT
    printf("<br>\n");
#endif
    makepic(fn1, fn);

    printf("\n</body></html>\n");
}

// addchat <rest of string>
void runaddchat(int cnt, char* str[]) {

}

// entry point
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Command must be passed. then list, quote or scene.\n");
        return 99;
    }

    int seed = (int)time(NULL);
    if (argc > 4) {
        seed = atoi(argv[4]);
    }
    srand(seed);
    printf("<!-- Seed: %d -->\n", seed);

    if (0 == strcmp(argv[1], "list")) {
        runlist();
    } else if (0 == strcmp(argv[1], "quote")) {
        if (argc < 3) {
            printf("Missing name of quoter\n");
            return 99;
        }
        runquote(argv[2]);
    } else if (0 == strcmp(argv[1], "scene")) {
        if (argc < 4) {
            printf("Missing name of both quoters\n");
            return 99;
        }
//        for (;;) 
        {
//            printf("<!-- Seed: %d -->\n", seed); srand(seed++);
            runscene(argv[2], argv[3]);
        }
    } else if (0 == strcmp(argv[1], "addchat")) {
        runaddchat(argc, argv);
    } else {
        printf("Did not recognize the command\n");
    }

    return 0;
}

