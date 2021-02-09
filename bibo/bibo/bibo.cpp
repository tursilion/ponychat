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
#include <string.h>
#include <vector>
#include <set>
using namespace std;
// up to three buffers - char 1, char 2, reply 1, reply 2
char* buf1, *buf2, *buf3, *buf4;
int len1, len2, len3, len4;
vector<string> nameList;    // always populated now
int trueNameListSize=0;     // number of entries from disk, no honorable mentions
vector<string> adjectives;  // adjective exceptions from the database
int replacedNamePos;        // global for the replaced name position
std::string replacedName;   // global for the replaced name

// enable this to make the text go left/right instead of all stacked on the left
// it was hard to make that work right so I want to save the code ;)
//#define TEXT_LEFTRIGHT

// enable this to test for sizing against preferred background and
// reference character Cheerilee (specify char number)
// (comment out, not 0, to disable)
// OR, define on commandline: -DGFX_TEST=1
//#define GFX_TEST 1

// where is the cgi?
#define CHAT_URL "000CHAT_URL000"

#ifdef _WIN32
#include <windows.h>
#define HONORMENS "D:\\work\\ponychat\\honorablementions.txt"
#define ADJECTIVES "D:\\work\\ponychat\\adjectives.txt"
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
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
DIR* dir;
struct dirent* d_ent;
string dirext;
#define HONORMENS "/home/ponychat/honorablementions.txt"
#define ADJECTIVES "/home/ponychat/adjectives.txt"
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

FILE *filopen(const char *path, const char *mode) {
    struct stat buf;
    if (stat(path, &buf)) {
        printf("<!-- no file -->\n");
        return NULL;
    }
    if (S_ISDIR(buf.st_mode)) {
        printf("<!-- dir -->\n");
        return NULL;
    }

    return fopen(path, mode);
}

string makefilename(string fn) {
    fn += '/';
    fn += getfilename();
    return fn;
}

#endif

// string to string case insensitive punctuation-ignoring only search
// replaces str.find
size_t findnocase(const string &str, const string &x, size_t start) {
  if (str.empty()) return string::npos;
  if (x.empty()) return string::npos;
  if (x.length() > str.length()) return string::npos;
  size_t match = string::npos;
  for (unsigned outer=start; outer<str.length()-x.length(); ++outer) {
    for (unsigned inner=0; inner<x.length(); ++inner) {
      char out = str[outer+inner];
      if (NULL != strchr("!?,.", out)) out = ' ';
      if (toupper(out) != toupper(x[inner])) {
        match = string::npos;
        break;
      }
      match=outer;
    }
    if (match != string::npos) {
//      printf("<!-- match '%s' in '%s' at %d ('%s') -->\n", x.c_str(), str.c_str(), match, str.substr(match).c_str());
      return match;
    }
  }
  return match; // empty
}

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
            char out = *p1;
            if (NULL != strchr("!?,.", out)) out = ' ';
            if (toupper(out) == toupper(*p2)) { ++p1; ++p2; continue; }
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
// warning - this ONLY matches a word with a space before it
const char* strrsearch(const char *base, const char* a, const char* b) {
    if ((a == NULL) || (b == NULL) || (base == NULL)) return NULL;
    while (a >= base) {
        const char* p1 = a;
        const char* p2 = b;
        {
            for (;;) {
                if ((*p1 == 0) || (*p2 == 0)) break;
                if ((*p2 <= '!') && (*p1 <= '!')) { ++p1; ++p2; continue; }
                if (toupper(*p1) == toupper(*p2)) { ++p1; ++p2; continue; }
                break;
            }
            if (*p2 == '\0') {
                return a;
            }
        }
        --a;
    }
    return NULL;
}

// attempt to guess a subject in the passed in string, return it if found
string findNoun(const string& str) {
  // we do a very simple and dumb pattern search to guess
  // we know the word before us always has a space after it, so that helps
  string outstr;
  for (string &x: adjectives) {
    size_t p = 0;
    do {
      p = findnocase(str, x, p);
      if (p != string::npos) {
        // make sure it's not the end of another word, unless it's "'s"
        if (x == "'s ") {
          // if it is 's, just make sure we didn't match "that's/he's/she's/it's", that's not possessive
          if ((p > 1) && (toupper(str[p-1]) == 'T') && (toupper(str[p-2]) == 'I')) {
            // no space, ignore match
            ++p;
            continue;
          }
          // one test is good enough for he and she
          if ((p > 1) && (toupper(str[p-1]) == 'E') && (toupper(str[p-2]) == 'H')) {
            // no space, ignore match
            ++p;
            continue;
          }
          if ((p > 3) && (toupper(str[p-1]) == 'T') && (toupper(str[p-2]) == 'A') && (toupper(str[p-3]) == 'H') && (toupper(str[p-4]) == 'T')) {
            // no space, ignore match
            ++p;
            continue;
          }
        } else {
          if ((p > 0) && (str[p-1] != ' ')) {
            // no space, ignore match
            ++p;
            continue;
          }
        }
        // special case: deny "the other" and "each other"
        if (x == "other ") {
          if ((p > 3) && (str.substr(p-4,4) == "the ")) {
            // no space, ignore match
            ++p;
            continue;
          }
          if ((p > 4) && (str.substr(p-5,5) == "each ")) {
            // no space, ignore match
            ++p;
            continue;
          }
        }
        // special case: deny "this is", "this was", "this for"
        if (x == "this ") {
          if (str.substr(p+4,4) == " is ") {
            // no space, ignore match
            ++p;
            continue;
          }
          if (str.substr(p+4,5) == " was ") {
            // no space, ignore match
            ++p;
            continue;
          }
          if (str.substr(p+4,5) == " for ") {
            // no space, ignore match
            ++p;
            continue;
          }
        }
        // build up the output word
        p += x.length();
        size_t p2 = str.find_first_of(" ,.?!", p);
        if (p2 == string::npos) {
          // no end of string?
          outstr = str.substr(p);
        } else {
          outstr = str.substr(p, p2-p);
        }
        return outstr;
      }
    } while (p != string::npos);
  }
  // last chance, a word ending in 'ness' is probably a noun...
  size_t p = findnocase(str, "ness", 0);
  if (p == string::npos) {
    return outstr; // empty
  }
  if (NULL == strchr(".,!? ", str[p+4])) {
    return outstr; // empty
  }
  // there MUST be a space before it
  p = str.rfind(' ', p);
  if (p == string::npos) {
    return outstr; // empty
  }
  // else, this is it...
  size_t p2 = str.find_first_of(" ,.?!", p);
  if (p2 == string::npos) {
    // no end of string?
    outstr = str.substr(p);
  } else {
    outstr = str.substr(p, p2-p);
  }
  return outstr;
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
// noun will cause a search for that word as a starting point, then clear it
string generateLine(char *buf1, int len1, char *buf2, int len2, string &noun) {
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
        int l2cnt = (len1 * 35 / 100) / len2; // how many times to copy buf2
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
    if (noun.length() > 0) {
        // try to find a noun match in buf1 only
        int x = findnocase(buf,noun,pos);
        if ((x > -1)&&(x < len1)) {
            if (x > 0) {
                // seek to start of line
                while (--x > 0) {
                  if (buf1[x] == '\n') { ++x; break; }
                }
            }
            // save it off and clear the noun
            printf("<!-- Found match for subject '%s' -->\n", noun.c_str());
            noun = "";
            pos = x;
        }
    }
    if ((pos > 0)&&(buf[pos-1] != '\n')) {
        // seek to the beginning of next line
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
    return rand() % trueNameListSize;    
}

// string replace - note that 'space' matches various punctuation
void strreplace(string& s, string src, string rep) {
    for (;;) {
        size_t p = findnocase(s, src, 0);
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
    // don't want punctuation on these hits, we are looking for mid-sentence
    size_t conjunct = findnocase(s, " and ", 0);
    if (conjunct == string::npos) conjunct = findnocase(s, " but ", 0);
    if (conjunct == string::npos) conjunct = findnocase(s, " if ", 0);
    if (conjunct == string::npos) conjunct = findnocase(s, " with ", 0);
    if (conjunct == string::npos) conjunct = findnocase(s, " how ", 0);

    for (;;) {
        size_t p = findnocase(s, src, 0);
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

    // look for broken replacements
    strreplace(s, " `me am ", " I am ");
    strreplace(s, "here am ", "here are "); // not necessarily a replacement, but that's okay

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
// note there must be at least one, or this won't work right
void populateNameList() {
    if (nameList.size() > 0) return;    // already loaded
    trueNameListSize = 0;

    if (!opendirect(SRCPATH, ".txt")) {
        printf("No dir for namelist\n");
        return;
    }

    do {
        string tmp = parseToName(getfilename());
        if (!tmp.empty()) {
            nameList.emplace_back(tmp);
            ++trueNameListSize;
        }
    } while (nextdir());
    klosedir();

    // a couple of honorable mentions... read in from file
    FILE *fp = filopen(HONORMENS, "r");
    if (NULL != fp) {
      while (!feof(fp)) {
        char buf[256];
        if (NULL == fgets(buf, sizeof(buf), fp)) break;
        while ((buf[0] != '\0') && (buf[strlen(buf)-1] < ' ')) buf[strlen(buf)-1]='\0';
        if (strlen(buf) > 1) nameList.emplace_back(buf);
      }
      fclose(fp);
    }

    // special-case adjectives for noun search... read in from file
    // this ends up also having the other special case words in them
    fp = filopen(ADJECTIVES, "r");
    if (NULL != fp) {
      while (!feof(fp)) {
        char buf[256];
        if (NULL == fgets(buf, sizeof(buf), fp)) break;
        while ((buf[0] != '\0') && (buf[strlen(buf)-1] < ' ')) buf[strlen(buf)-1]='\0';
        if (strlen(buf) >= 1) adjectives.emplace_back(buf);
      }
      fclose(fp);
    }
    // now post process and put a space after all of them
    for (string& x: adjectives) {
      x += ' ';
    }

    // debug
//    for (string x : nameList) {
//        printf("<!-- '%s' -->\n", x.c_str());
//    }
}

// replace a potential name tstname in str with n at p
// return true if we did it
bool replaceName(const string &tstname, string &str, const string &n, size_t p) {
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
                do {
                    // check for extra spaces
                    op = on.find(' ');
                    if (op == string::npos) break;
                    on = on.substr(op+1);
                } while (1);
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
    else if (on == "On") on = n;        // just "On" doesn't make sense (On Stage)
    else if (on == "Old") on = n;       // just "Old" doesn't make sense (Old Man Mcgucket)

    // and do the replace
    string first;
    if ((p > 0)&&(p+tstname.length() < str.length())) {
        first = str.substr(0,p);
    }
    str = first + on + str.substr(p+tstname.length());
    printf("<!-- replace '%s' with '%s' -->\n", tstname.c_str(), on.c_str());

    replacedName = on;
    replacedNamePos = p;

    return true;
}

// find x in str, and ensure it is either first word or
// after a comma, and has punctuation right after it
// (so that it's likely calling the other party, rather
// than referring to a third person).
// Nosplit returns true if a name match is prefixed with
// 'a' or 'the' and so rejected, so that the caller
// doesn't try splitting the name
// namefind SHOULD be case-sensitive!!
size_t namefind(string &str, string &x, bool &nosplit) {
    size_t p = string::npos;
    nosplit = false;
    const int debug = 0;

    // little hacky, but disallow some bad names
    if (x == "Mark Crusaders") { return p; }
    if (x == "Pony") { return p; }
    if (x == "Lord") { return p; }

    // first, any match at all?
    p = str.find(x, 0);
    if (string::npos == p) { return p; }

    if (debug) printf("<!-- matched '%s' in '%s' -->\n", x.c_str(), str.c_str());

    // now, is it a desired match?
    // make sure we start clean - either a space or start of line before
    if ((p != 0)&&(str[p-1] != ' ')) {
        if (debug) printf("<!-- not start of line or preceded by space -->\n");
        return string::npos;  // explicitly space
    }

    // make sure it was not "a name", such as "a princess"
    if ((p >= 3)&&(tolower(str[p-2]) == 'a')&&(str[p-3]==' ')) { 
      if (debug) printf("<!-- reject 'a' name -->\n"); 
      nosplit=true; 
      return string::npos; 
    }
    // also 'the'
    if ((p >= 5)&&(tolower(str[p-2]) == 'e')&&(tolower(str[p-3])=='h')&&(tolower(str[p-4])=='t')&&(str[p-5]==' ')) {
      if (debug) printf("<!-- reject 'the' name -->\n"); 
        nosplit = true;
        return string::npos;
    }

    // post-comma makes it okay (end of phrase)
    if (NULL != strchr(",", str[p+x.length()])) {
        if (debug) printf("<!-- ACCEPT: followed by comma -->\n");
        return p;
    }

    // if no punctuation, it has to be a space or apostrophe after, otherwise we are part of another word
    // (apostrophe for possessive (name's thing))
    if (NULL == strchr(" '!?.", str[p+x.length()])) {
      if (debug) printf("<!-- reject: not followed by space or puncutation -->\n");
      return string::npos;
    }

    // start of line is okay if that's all there is (like 'Rainbow Dash!')
    if ((p == 0)&&(strchr("!?.", str[p+x.length()]))) {
        if (debug) printf("<!-- ACCEPT: name is entire line -->\n");
        return p;
    }

    // after comma is okay, if entire phrase. (] is also treated as comma)
    // "If I'm not there, Tree Hugger is responsible for you" is not a good replacement
    // but "Hold on, Simba!" is.
    if ((p > 1) && (str[p-1] == ' ') && (strchr("],", str[p-2]))) {
        // first part is okay
        if (NULL != strchr("!?.", str[p+x.length()])) {
            if (debug) printf("<!-- ACCEPT: name is entire phrase -->\n");
            return p;
        }
    }

    if (debug) printf("<!-- Reject: no accept cases found. -->\n");

    // else it's probably a third party reference
    return string::npos;
}


// perform name substitution in the string str, us is our name
// n is the name of the other character, if we decide to use it
void nameSubstitution(string &str, const string &n, const string &us) {
    // Rules: It might be a name to substitute only if it's the
    // first or the last. Punctuation is used to verify. (If it's
    // in the middle, it's more likely a third party).
    string tstname;

    // make sure beginning and end have no spaces
    while ((str.length())&&(str[0] == ' ')) str = str.substr(1);
    while ((str.length())&&(str[str.length()-1] == ' ')) str=str.substr(0,str.length()-1);

    // go through the name list, and replace only first or last words in the sentence,
    // with separating punctuation
    // First pass we search ONLY complete names, second pass we split it up. This
    // helps prevent ordering issues (for instance "Diamond" might match for "Diamond Tiara"
    // before "Double Diamond", even if "Double Diamond" is what was in the text)
    // We also prefer the earliest match in case there are multiple.
    // this also lets us NOT split the honorable mention names, they must fully match
    size_t finalp = string::npos;
    size_t finall = 0;
    string finaltstname;
    int maxpass = 2;

    for (int pass = 0; pass < maxpass; ++pass) {
        int nameIdx = 0;
        for (string x : nameList) {
            if (x == us) continue;  // don't replace self references
            if ((pass > 0) && (nameIdx >= trueNameListSize)) break; // don't split up honorable mentions
            ++nameIdx;

            size_t p = string::npos;
            size_t l = 0;
            string tstname;

            if (pass == 0) {
                bool nosplit = false;
                p = namefind(str,x,nosplit);
                if (nosplit) maxpass = 1; // even though this will affect all names, it's okay

                if ((p != string::npos) && ((finalp == string::npos)||(p < finalp))) {
                    finaltstname = x;
                    finall = x.length();
                    finalp = p;
                }
            } else {
                bool xx = false;  // dummy value on this pass
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
                            p = namefind(str,tstname,xx);
                            l = tstname.length();
                            if ((p != string::npos) && ((finalp == string::npos)||(p < finalp))) {
                                finalp = p;
                                finall = l;
                                finaltstname = tstname;
                            }
                        }

                        // try Ms/Mrs/Mr lastname
                        if (p == string::npos) {
                            tstname = "Ms " + n2;
                            p = namefind(str,tstname,xx);
                            l = tstname.length();
                            if ((p != string::npos) && ((finalp == string::npos)||(p < finalp))) {
                                finalp = p;
                                finall = l;
                                finaltstname = tstname;
                            }
                        }
                        if (p == string::npos) {
                            tstname = "Mrs " + n2;
                            p = namefind(str,tstname,xx);
                            l = tstname.length();
                            if ((p != string::npos) && ((finalp == string::npos)||(p < finalp))) {
                                finalp = p;
                                finall = l;
                                finaltstname = tstname;
                            }
                        }
                        if (p == string::npos) {
                            tstname = "Miss " + n2;
                            p = namefind(str,tstname,xx);
                            l = tstname.length();
                            if ((p != string::npos) && ((finalp == string::npos)||(p < finalp))) {
                                finalp = p;
                                finall = l;
                                finaltstname = tstname;
                            }
                        }
                        if (p == string::npos) {
                            tstname = "Mr " + n2;
                            p = namefind(str,tstname,xx);
                            l = tstname.length();
                            if ((p != string::npos) && ((finalp == string::npos)||(p < finalp))) {
                                finalp = p;
                                finall = l;
                                finaltstname = tstname;
                            }
                        }

                        // first name
                        if (p == string::npos) {
                            // skip some first names that are more likely to be nouns
                            // needs to be in a file list... or tag the names somehow
                            for (;;) {
                                if (n1 == "Trouble") break;
                                if (n1 == "Apple") break;
                                if (n1 == "Big") break;
                                if (n1 == "Clear") break;
                                if (n1 == "Cozy") break;
                                if (n1 == "Iron") break;
                                if (n1 == "Lightning") break;
                                if (n1 == "Night") break;
                                if (n1 == "Photo") break;
                                if (n1 == "Sour") break;
                                if (n1 == "Spoiled") break;
                                if (n1 == "Tree") break;
                                if (n1 == "Wind") break;
                                if (n1 == "Windy") break;
                                if (n1 == "Tree") break;
                                if (n1 == "On") break;

                                tstname = n1;
                                p = namefind(str,n1,xx);
                                l = n1.length();
                                if ((p != string::npos) && ((finalp == string::npos)||(p < finalp))) {
                                    finalp = p;
                                    finall = l;
                                    finaltstname = tstname;
                                }
                                break;
                            }
                        }

                        // last name 
                        if (p == string::npos) {
                            // skip some last names that are more likely to be nouns
                            // needs to be in a file list...
                            for (;;) {
                                if (n2 == "King") break;
                                if (n2 == "Shoes") break;
                                if (n2 == "Dazzle") break;
                                if (n2 == "Bunny") break;
                                if (n2 == "Bloom") break;
                                if (n2 == "Rose") break;
                                if (n2 == "Blaze") break;
                                if (n2 == "Seed") break;
                                if (n2 == "Sandwich") break;
                                if (n2 == "Sky") break;
                                if (n2 == "Glow") break;
                                if (n2 == "Donkey") break;
                                if (n2 == "Do") break;
                                if (n2 == "Horse") break;
                                if (n2 == "Pages") break;
                                if (n2 == "Sentry") break;
                                if (n2 == "Heart") break;
                                if (n2 == "Daisy") break;
                                if (n2 == "Delicious") break;
                                if (n2 == "Gruff") break;
                                if (n2 == "Pear") break;
                                if (n2 == "Will") break;
                                if (n2 == "Montage") break;
                                if (n2 == "Dust") break;
                                if (n2 == "Pie") break;
                                if (n2 == "Mare") break;
                                if (n2 == "Skies") break;
                                if (n2 == "Dancer") break;
                                if (n2 == "Cake") break;
                                if (n2 == "Light") break;
                                if (n2 == "Moon") break;
                                if (n2 == "Favor") break;
                                if (n2 == "Petals") break;
                                if (n2 == "Finish") break;
                                if (n2 == "Shadows") break;
                                if (n2 == "Pants") break;
                                if (n2 == "Saddles") break;
                                if (n2 == "Armor") break;
                                if (n2 == "Sweet") break;
                                if (n2 == "Magnet") break;
                                if (n2 == "Shadow") break;
                                if (n2 == "Taps") break;
                                if (n2 == "Spruce") break;
                                if (n2 == "Trail") break;
                                if (n2 == "Blush") break;
                                if (n2 == "Sprint") break;
                                if (n1 == "Whistles") break;
                                if (n1 == "Breeze") break;
                                if (n1 == "Gourmand") break;
                                if (n1 == "Stage") break;

                                tstname = n2;
                                p = namefind(str,n2,xx);
                                l = n2.length();
                                if ((p != string::npos) && ((finalp == string::npos)||(p < finalp))) {
                                    finalp = p;
                                    finall = l;
                                    finaltstname = tstname;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        }
        // don't do the second pass if the first pass was successful
        if (finalp != string::npos) break;
    }

    // if no match in the end, return
    if (finalp == string::npos) return;

    // Disallow certain parts of names that don't work on their own
    if (finaltstname == "Cutie") return;  // Cutie Mark Crusaders
    if (finaltstname == "Mark") return;  // Cutie Mark Crusaders

    // Finally, if we are referring to ourselves, don't replace it
    if (string::npos != findnocase(us, finaltstname, 0)) {
        printf("<!-- Don't replace '%s' because it's us -->\n", finaltstname.c_str());
        return;
    }

    // there is a match, check start or end, and full name! (Cause 'Ma' is short)
    // must be followed by punctuation, and must be either the start of the
    // line, or preceded by a comma.
    replaceName(finaltstname, str, n, finalp);
}

// a class for runList to sort the chars with
class Foo {
public:
    Foo(const string &s, int i) : str(s), idx(i) {};
    bool operator<(const Foo &foo) const { return str < foo.str; }
    const string str;
    int idx;
};

// list (no arg)
void runlist() {
    set<Foo> s;
    // walk the list manually cause we need to skip the honorable mentions
    for (int idx = 0; idx<trueNameListSize; ++idx) {
        s.insert(Foo(nameList[idx], idx+1));
    }

    if (!opendirect(SRCPATH, ".txt")) {
        printf("no dir\n");
        return;
    }

    for (Foo x : s) {
        printf("\n<li><a href=\"%s?%d\">%s</a></li>   \n", CHAT_URL, x.idx, x.str.c_str());
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
void runquote(int who, int count) {
    int w = who;
    if ((w == 0) || (w > trueNameListSize)) {
        w = randomfile();
    }
    if (!opendirect(SRCPATH, ".txt")) {
        printf("No dir\n");
    }
    printf("\n<html><body>\n");
    addstyle();

    while (--w > 0) {
        if (!nextdir()) break;
    }

    // print out the name of the character, from the filename
    std::string fn = getfilename();
    std::string un2 = parseToName(fn);
    printf("<p class=\"talk2\">\n");
    printf("<b>%s: </b>", un2.c_str());

    // suck the file into memory
    fn = makefilename(SRCPATH);
    klosedir();

    FILE* fp = filopen(fn.c_str(), "r");
    if (NULL == fp) {
        printf("No file %s\n", fn.c_str());
        return;
    }
    fseek(fp, 0, SEEK_END);
    len1 = ftell(fp);
    if ((len1 < 1) || (len1 > 512*1024)) {
        printf("<!-- invalid filesize -->\n");
        fclose(fp);
        return;
    }
    ++len1;
    fseek(fp, 0, SEEK_SET);
    buf1 = (char*)malloc(len1 + 2);
    if (NULL == buf1) {
        printf("<!-- no mem -->\n");
        fclose(fp);
        return;
    }
    buf1[0] = '\n';
    len1 = (int)fread(buf1 + 1, 1, len1, fp);
    fclose(fp);
    buf1[len1] = '\n';
    buf1[len1 + 1] = '\0';
    ++len1;
    // special case for databases with a blank lines
    fixbuf(buf1, len1);

    // now start babbling
    int cnt = count;
    if ((count <= 0) || (count > 10)) {
        cnt = rand() % 5 + 2;
    }
    for (int idx = 0; idx < cnt; ++idx) {
        string dummy;
        string s = generateLine(buf1, len1, NULL, 0, dummy);
        printf("%s", s.c_str());
        if (s.empty()) --idx;   // if there's a blank line in the database, then we get an empty output. Ignore it.
    }
    printf("</p>\n<br><br>\n");

    // and finally, generate the bottom image
    makepic(fn, "");

    printf("\n</body></html>\n");

}

// scene <char1> <char2>
// who1, who2 - indexes of characters to chat
// count - number of lines to generate
// count2 - maximum number of lines per character line
// any may be zero for random
void runscene(int who1, int who2, int count, int count2) {
    int w = who1;
    if ((w == 0) || (w > trueNameListSize)) {
        w = randomfile();
    }
#ifdef GFX_TEST
    w = 89; // preferred reference is Cheerilee (check for changes)
#endif
    //printf("<!-- %d -->\n", w);

    if (!opendirect(SRCPATH, ".txt")) {
        printf("No dir\n");
    }

    int wold = w;
    while (--w > 0) {
        if (!nextdir()) break;
    }

    // work out the name of the character, from the filename
    std::string fn = getfilename();
    string un1 = parseToName(fn);
    if (un1.empty()) {
        printf("Character parse failed, sorry.\n");
        return;
    }

    // suck the file into memory
    fn = makefilename(SRCPATH);
    klosedir();

    FILE* fp = filopen(fn.c_str(), "r");
    if (NULL == fp) {
        printf("No file %s\n", fn.c_str());
        return;
    }
    fseek(fp, 0, SEEK_END);
    len1 = ftell(fp);
    if ((len1 < 1) || (len1 > 512*1024)) {
        printf("<!-- invalid filesize2 -->\n");
        fclose(fp);
        return;
    }
    ++len1;
    fseek(fp, 0, SEEK_SET);
    buf1 = (char*)malloc(len1 + 2);
    if (NULL == buf1) {
        printf("<!-- no mem2 -->\n");
        fclose(fp);
        return;
    }
    buf1[0] = '\n';
    len1 = (int)fread(buf1 + 1, 1, len1, fp);
    fclose(fp);
    buf1[len1] = '\n';
    buf1[len1 + 1] = '\0';
    ++len1;
    // special case for databases with a blank lines
    fixbuf(buf1, len1);

    // now babbler 2
    w = who2;
    if ((w == 0) || (w > trueNameListSize)) {
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
        if (!nextdir()) break;
    }

    // save the name
    string fn1 = fn;

    // work out the name of the character, from the filename
    fn = getfilename();
    string un2 = parseToName(fn);
    if (un2.empty()) {
        printf("Character parse2 failed, sorry.\n");
        return;
    }

    // suck the file into memory
    fn = makefilename(SRCPATH);
    klosedir();

    fp = filopen(fn.c_str(), "r");
    if (NULL == fp) {
        printf("No file %s\n", fn.c_str());
        return;
    }
    fseek(fp, 0, SEEK_END);
    len2 = ftell(fp);
    if ((len2 < 1) || (len2 > 512*1024)) {
        printf("<!-- invalid filesize3 -->\n");
        fclose(fp);
        return;
    }
    ++len2;
    fseek(fp, 0, SEEK_SET);
    buf2 = (char*)malloc(len2 + 2);
    if (NULL == buf2) {
        printf("<!-- no mem3 -->\n");
        fclose(fp);
        return;
    }
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
    string globalnoun1,globalnoun2;
    int lps = count;
    if ((lps < 1) || (lps > 10)) {
        lps = rand() % 4 + 2;
    }
    for (int lp = 0; lp < lps; ++lp) {
        int cnt;
        if ((count2 < 1) || (count2 > 10)) {
            cnt = rand() % 1 + 1;
        } else {
            cnt = rand() % count2 + 1;
        }
        if (lp & 1) {
#ifdef TALK_LEFTRIGHT
            printf("<p class=\"talkpad\">&nbsp</p>");
#endif
            printf("<p class=\"talk2\">\n");
            printf("<b>%s: </b>", un2.c_str());
            for (int idx = 0; idx < cnt; ++idx) {
                string s = generateLine(buf2, len2, buf4, len4, globalnoun1);
                if (s.empty()) {
                    // if there's a blank line in the database, then we get an empty output. Ignore it.
                    --idx;
                    continue;
                }
                // noun testing...
                string noun = findNoun(s);
                printf("<!-- New subject guess: '%s' -->\n", noun.c_str());
                if (!noun.empty()) globalnoun2 = noun;
                ////
                replacedNamePos = -1;
                nameSubstitution(s, un1, un2);
                printf("%s ", s.c_str());
                s += '\n';
                // if there was a replaced name, change it to us in case they reply with it
                if (replacedNamePos > -1) {
                    replaceName(replacedName, s, un2, replacedNamePos);
                }
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
            //globalnoun1 = "";
        } else {
            printf("<p class=\"talk1\">\n");
            printf("<b>%s: </b>", un1.c_str());
            for (int idx = 0; idx < cnt; ++idx) {
                string s = generateLine(buf1, len1, buf3, len3, globalnoun2);
                if (s.empty()) {
                    // if there's a blank line in the database, then we get an empty output. Ignore it.
                    --idx;
                    continue;
                }
                // noun testing
                string noun = findNoun(s);
                printf("<!-- New subject guess: '%s' -->\n", noun.c_str());
                if (!noun.empty()) globalnoun1 = noun;
                ////
                replacedNamePos = -1;
                nameSubstitution(s, un2, un1);
                printf("%s ", s.c_str());
                s += '\n';
                // if there was a replaced name, change it to us in case they reply with it
                if (replacedNamePos > -1) {
                    replaceName(replacedName, s, un1, replacedNamePos);
                }
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
            //globalnoun2 = "";
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
// args: scene c1 c2 cnt seed
int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Command must be passed. then list, quote or scene.\n");
        return 99;
    }

    // get the names from the disk
    populateNameList();

    // get char1
    int c1 = 0;
    if (argc > 2) {
        int c = atoi(argv[2]);
        if (c > 0) c1 = c;
    }

    // get char2
    int c2 = 0;
    if (argc > 3) {
        int c = atoi(argv[3]);
        if (c > 0) c2 = c;
    }

    // get count
    int cnt = 0;
    if (argc > 4) {
        int c = atoi(argv[4]);
        if (c > 0) cnt = c;
    }

    // get count2
    int cnt2 = 0;
    if (argc > 5) {
        int c = atoi(argv[5]);
        if (c > 0) cnt2 = c;
    }

    // set up a seed
    int seed = (int)time(NULL);
    if (argc > 6) {
        int s = atoi(argv[6]);
        if (s > 0) seed=s;
    }
    srand(seed);
    printf("<!-- Trigger String: %d.%d.%d.%d.%d -->\n", c1,c2, cnt,cnt2, seed);

    if (0 == strcmp(argv[1], "list")) {
        runlist();
    } else if (0 == strcmp(argv[1], "quote")) {
        if (argc < 3) {
            printf("Missing name of quoter\n");
            return 99;
        }
        runquote(c1, cnt);
    } else if (0 == strcmp(argv[1], "scene")) {
        if (argc < 4) {
            printf("Missing name of both quoters\n");
            return 99;
        }
        runscene(c1, c2, cnt, cnt2);
    } else if (0 == strcmp(argv[1], "addchat")) {
        runaddchat(argc, argv);
    } else {
        printf("Did not recognize the command\n");
    }

    return 0;
}

