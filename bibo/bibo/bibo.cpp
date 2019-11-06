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
using namespace std;
// up to three buffers - char 1, char 2, reply 1, reply 2
char* buf1, * buf2, * buf3, *buf4;
int len1, len2, len3, len4;

#define CHAT_URL "http://harmlesslion.com/ponychat/ponychat.cgi"

#ifdef _WIN32
#include <windows.h>
#define SRCPATH "C:\\work\\ponychat\\SeparateChars\\*.txt"
#define IMGPATH "C:\\work\\ponychat\\images\\*.png"
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
    if (x.substr(x.length()-4, 4) != dirext) continue;
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

// case insensitive and whitespace agnostic substring search
const char* strsearch(const char* a, const char* b) {
	if ((a == NULL) || (b == NULL)) return NULL;
	int len = strlen(b);
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
	}
	return NULL;
}

// add the style tags needed for the pic
void addstyle() {
  printf("<style>\n.parent{\n  position: relative;\n  top:0;\n  left:0;\n  right:100%\n  z-index:0;\n}\n");
  printf(".over-img1{\n  position:absolute;\n  bottom: 5%;\n  left: 5%;\n  z-index:1;\n}\n");
  printf(".over-img2{\n  position:absolute;\n  bottom: 5%;\n  right: 5%;\n z-index:1;\n  transform: scaleX(-1);\n}\n");
  printf(".over-img3{\n  position:absolute;\n  bottom: 5%;\n  left: 40%;\n z-index:1;\n}\n");
  printf(".torch-img1{\n  position:absolute;\n  bottom: -28%;\n  left: 5%;\n  z-index:1;\n}\n");
  printf(".torch-img2{\n  position:absolute;\n  bottom: -28%;\n  right: 5%;\n z-index:1;\n  transform: scaleX(-1);\n}\n");
  printf(".torch-img3{\n  position:absolute;\n  bottom: -28%;\n  left: 40%;\n z-index:1;\n}\n");

  printf("</style>\n");
}

// generate the bottom picture...
void makepic(const string &fn1, const string &fn2) {
    const int numpics = 20;
    string bgname, name1, name2;
    if (!opendirect(IMGPATH, ".png")) return;

    printf("<div class=\"parent\">\n");

    int img=rand()%numpics+1;
    char buf[1024];
    sprintf(buf, "bg%d_by_", img);
    printf("<!-- %s -->\n", buf);

    for (;;) {
      if (getfilename().substr(0,strlen(buf)) == buf) {
        // got it!
        bgname = getfilename();
        printf("<img width=\"100%\" src=\"/ponyimages/%s\">\n", bgname.c_str());
        break;
      }
      if (!nextdir()) break;
    }
    klosedir();

    string clss;
    if (fn2.empty()) {
      clss="over-img3";
    } else {
      clss="over-img1";
    }

    strncpy(buf, fn1.c_str(), sizeof(buf));
    buf[sizeof(buf)-1]='\0';
    char *p = strrchr(buf, '.');
    if (p) *p='\0';
    p = strrchr(buf, '/');
    if (p) memmove(buf, p+1, strlen(p));
    p = strrchr(buf, '\\');
    if (p) memmove(buf, p+1, strlen(p));
    printf("<!-- %s -->\n", buf);
    if (0 == strcmp(buf, "DragonLordTorch")) clss.replace(0,4,"torch");

    if (opendirect(IMGPATH, ".png")) {
      for (;;) {
        if (getfilename().substr(0,strlen(buf)) == buf) {
          // got it
          name1=getfilename();
          if (0 == strcmp(buf, "CutieMarkCrusaders")) {
            printf("<img width=\"50%\" src=\"/ponyimages/%s\" class=\"%s\">\n", name1.c_str(), clss.c_str());
          } else {
            printf("<img width=\"25%\" src=\"/ponyimages/%s\" class=\"%s\">\n", name1.c_str(), clss.c_str());
          }
          break;
        }
        if (!nextdir()) break;
      }
      klosedir();
    }

    if (!fn2.empty()) {
      clss = "over-img2";

      strncpy(buf, fn2.c_str(), sizeof(buf));
      buf[sizeof(buf)-1]='\0';
      char *p = strrchr(buf, '.');
      if (p) *p='\0';
      p = strrchr(buf, '/');
      if (p) memmove(buf, p+1, strlen(p));
      p = strrchr(buf, '\\');
      if (p) memmove(buf, p+1, strlen(p));
      printf("<!-- %s -->\n", buf);
      if (0 == strcmp(buf, "DragonLordTorch")) clss.replace(0,4,"torch");

      if (opendirect(IMGPATH, ".png")) {
        for (;;) {
          if (getfilename().substr(0,strlen(buf)) == buf) {
            // got it
            name2=getfilename();
            if (0 == strcmp(buf, "CutieMarkCrusaders")) {
              printf("<img width=\"50%\" src=\"/ponyimages/%s\" class=\"%s\">\n", name2.c_str(), clss.c_str());
            } else {
              printf("<img width=\"25%\" src=\"/ponyimages/%s\" class=\"%s\">\n", name2.c_str(), clss.c_str());
            }
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
	while ((pos < len) && (buf[pos]==' ')) ++pos;
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

	if (buf2 == NULL) {
		if (len1 > buflen) {
			buf = (char*)realloc(buf, len1 + 1);
			buf[len1] = '\0';
			buflen = len1;
		}
		memcpy(buf, buf1, len1);
		buf[len1] = 0;
		len = len1;
	}
	else {
		// we want to kind of balance out the chat log with the source text...
                // 50:50 was cute but repetitive, let's try 75:25
		int l2cnt = (len1/2) / len2;
		if (l2cnt == 0) l2cnt = 1;
		if (len1+len2*l2cnt > buflen) {
			buf = (char*)realloc(buf, len1+len2*l2cnt + 1);
			buf[len1+len2*l2cnt] = '\0';
			buflen = len1+len2*l2cnt;
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
	if (pos == 0) pos = 1;
	if (pos > 1) {
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
			if ((w[w.length()-1]==']')&&(w[0]!='[')) {
				// remove close braces without an open
				int p=output.length()-2; // -2 to skip the bracket we found
				while (p >= 0) {
					if (output[p]=='[') break;
					if (output[p]==']') p=0;
					--p;
				}
				if (p < 0) {
					output += w.substr(0,w.length()-1) + ' ';
					continue;
				}
			}
			output += w + ' ';
		}

		// if we have no word, then exit
		if (w.empty()) break;
		// if we have punctuation (except comma), then we ended on an end word
		if ((w[w.length() - 1] < 'A')&& (w[w.length() - 1] != ',')) break;

		// find a new instance of this same word
		pos = rand() % len; 
		w = ' ' + w + ' ';
		const char* p = strsearch(&buf[pos], w.c_str());
		if (NULL == p) {
			p = strsearch(buf, w.c_str());
			if (NULL == p) {
				output += "then I lost my place! ";
				goto finish;
			}
		}

		// skip to the end of the word, then loop
		pos = p - buf + w.length();
	}

finish:
        // look for open brace with no close, and add one (asides, etc)
        bool brace=false;
        for (int idx=0; idx<output.length(); ++idx) {
          if (output[idx]=='[') {
              if (brace) {
		// nested brace, fix it
		output.replace(idx,1,"][");
                brace = false;
              } else {
                brace = true;
              }
          } else if (output[idx]==']') {
              brace = false;
          }
        }
	if (brace) {
	    output[output.length()-1] = ']';
            output += ' ';
	}

	// capitalize first letter
	output[0] = toupper(output[0]);

	// if no punctuation at end, add one (space also at end)
	if (NULL == strchr(".!?]", output[output.length()-2])) {
           output[output.length()-1] = '.';
           output += ' ';
        }
	printf("%s", output.c_str());
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
		}
		else {
			if (p < conjunct) {
				if (p >= conjunct / 2) replace = rep2;
			}
			else {
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

	s = s.substr(1, s.length() - 2);
	s[0] = toupper(s[0]);
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
	for(;;) {
		printf("\n<a href=\"%s?%d\">%s</a>   \n", CHAT_URL, cnt++, getfilename().c_str());
		if (!nextdir()) {
			klosedir();
			printf("</body></html>");
			return;
		}
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
	buf1 = (char*)malloc(len1+2);
	buf1[0] = '\n';
	len1=fread(buf1+1, 1, len1, fp);
	fclose(fp);
	buf1[len1] = '\n';
	buf1[len1+1] = '\0';
	++len1;

	// now start babbling
	int cnt = rand() % 5 + 2;
	for (int idx = 0; idx < cnt; ++idx) {
		generateLine(buf1, len1, NULL, 0);
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
	if (!opendirect(SRCPATH, ".txt")) {
		printf("No dir\n");
	}

        int wold=w;
	while (--w > 0) {
		nextdir();
	}

	// work out the name of the character, from the filename
	std::string fn = getfilename();
	string un1;
	for (unsigned int idx = 0; idx < fn.length(); ++idx) {
		if (fn[idx] == '.') break;
		if ((idx > 0) && (fn[idx] >= 'A') && (fn[idx] <= 'Z')) un1 += ' ';
		un1+= fn[idx];
	}
	un1+=": ";

	// suck the file into memory
	fn = makefilename(SRCPATH);
	klosedir();

	FILE* fp = filopen(fn.c_str(), "r");
	if (NULL == fp) {
		printf("No file %s\n", fn.c_str());
		return;
	}
	fseek(fp, 0, SEEK_END);
	len1 = ftell(fp)+1;
	fseek(fp, 0, SEEK_SET);
	buf1 = (char*)malloc(len1 + 2);
	buf1[0] = '\n';
	len1=fread(buf1 + 1, 1, len1, fp);
	fclose(fp);
	buf1[len1] = '\n';
	buf1[len1+1] = '\0';
	++len1;

	// now babbler 2
        while (wold == w) {
 	 	w = atoi(who1);
 		if (w == 0) {
			w = randomfile();
		}
	}
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
	string un2;
	for (unsigned int idx = 0; idx < fn.length(); ++idx) {
		if (fn[idx] == '.') break;
		if ((idx > 0) && (fn[idx] >= 'A') && (fn[idx] <= 'Z')) un2 += ' ';
		un2 += fn[idx];
	}
	un2 += ": ";

	// suck the file into memory
	fn = makefilename(SRCPATH);
	klosedir();

	fp = filopen(fn.c_str(), "r");
	if (NULL == fp) {
		printf("No file %s\n", fn.c_str());
		return;
	}
	fseek(fp, 0, SEEK_END);
	len2 = ftell(fp)+1;
	fseek(fp, 0, SEEK_SET);
	buf2 = (char*)malloc(len2 + 2);
	buf2[0] = '\n';
	len2=fread(buf2 + 1, 1, len2, fp);
	fclose(fp);
	buf2[len2] = '\n';
	buf2[len2+1] = '\0';
	++len2;

	printf("\n<html><body>\n");
	addstyle();

	// now start babbling
	len3 = 0;
	len4 = 0;
	int lps = rand() % 4 + 2;
	for (int lp = 0; lp < lps; ++lp) {
		int cnt = rand() % 1 + 1;
		if (lp & 1) {
			printf("%s", un2.c_str());
			for (int idx = 0; idx < cnt; ++idx) {
				string s = generateLine(buf2, len2, buf4, len4) + '\n';
				// add the string to the chat
				fixpronouns(s);
				buf3 = (char*)realloc(buf3, len3 + 1 + s.length());
				buf3[len3] = 0;
				if (NULL == buf3) {
					printf("realloc failed\n");
					return;
				}
				strcat(&buf3[len3], s.c_str());
				len3 += s.length();
			}
		}
		else {
			printf("%s", un1.c_str());
			for (int idx = 0; idx < cnt; ++idx) {
				string s = generateLine(buf1, len1, buf3, len3) + '\n';
				// add the string to the chat
				fixpronouns(s);
				buf4 = (char*)realloc(buf4, len4 + 1 + s.length());
				buf4[len4] = 0;
				if (NULL == buf4) {
					printf("realloc failed\n");
					return;
				}
				strcat(&buf4[len4], s.c_str());
				len4 += s.length();
			}
		}
		printf("\n<br><br>\n");
	}

	// bottom image
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
	srand(seed);
	printf("<!-- Seed: %d -->\n", seed);

	if (0 == strcmp(argv[1], "list")) {
		runlist();
	}
	else if (0 == strcmp(argv[1], "quote")) {
		if (argc < 3) {
			printf("Missing name of quoter\n");
			return 99;
		}
		runquote(argv[2]);
	}
	else if (0 == strcmp(argv[1], "scene")) {
		if (argc < 4) {
			printf("Missing name of both quoters\n");
			return 99;
		}
		runscene(argv[2], argv[3]);
	}
	else if (0 == strcmp(argv[1], "addchat")) {
		runaddchat(argc, argv);
	}
	else {
		printf("Did not recognize the command\n");
	}

	return 0;
}

