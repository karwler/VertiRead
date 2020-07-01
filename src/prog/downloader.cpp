#ifdef BUILD_DOWNLOADER
#include "engine/world.h"

// COMIC

Comic::Comic(string capt, vector<pairStr>&& chaps) :
	title(std::move(capt)),
	chapters(std::move(chaps))
{}

// WEB SOURCE

WebSource::WebSource(CURL* curlm, SDL_mutex* mlock, CURL* cfile) :
	curlMain(curlm),
	mainLock(mlock),
	curlFile(cfile)
{}

WebSource::Type WebSource::source() const {
	return Type(UINT8_MAX);
}

xmlDoc* WebSource::downloadHtml(const string& url) {
	string data;
	while (SDL_TryLockMutex(mainLock));
	curl_easy_setopt(curlMain, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curlMain, CURLOPT_WRITEDATA, &data);
	CURLcode code = curl_easy_perform(curlMain);
	SDL_UnlockMutex(mainLock);

	if (code) {
		std::cerr << "CURL error: " << curl_easy_strerror(code) << std::endl;
		return nullptr;
	}
	return htmlReadDoc(reinterpret_cast<const xmlChar*>(data.c_str()), url.c_str(), nullptr, HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING);
}

DownloadState WebSource::downloadFile(const string& url, const fs::path& drc) {
	fs::path fpath = drc / FileSys::validateFilename(fs::u8path(std::find_if(url.rbegin(), url.rend(), isDsep).base(), url.end()));
#ifdef _WIN32
	FILE* ofh = _wfopen(fpath.c_str(), L"wb");
#else
	FILE* ofh = fopen(fpath.c_str(), "wb");
#endif
	if (!ofh)
		return World::downloader()->getDlState();

	curl_easy_setopt(curlFile, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curlFile, CURLOPT_WRITEDATA, ofh);
	CURLcode code = curl_easy_perform(curlFile);
	fclose(ofh);
	if (code) {
		std::cerr << "CURL error: " << curl_easy_strerror(code) << std::endl;
		remove(fpath.c_str());
	}
	return World::downloader()->getDlState();
}

string WebSource::toUrl(string str) {
	while (SDL_TryLockMutex(mainLock));
	char* ret = curl_easy_escape(curlMain, str.c_str(), int(str.length()));
	SDL_UnlockMutex(mainLock);

	if (ret) {
		str = ret;
		curl_free(ret);
		return str;
	}
	return string();
}

xmlNode* WebSource::findElement(xmlNode* node, const char* tag) {
	for (xmlNode* cur = node->children; cur; cur = cur->next) {
		if (namecmp(cur->name, tag))
			return cur;
		if (xmlNode* ret = findElement(cur, tag))
			return ret;
	}
	return nullptr;
}

vector<xmlNode*> WebSource::findElements(xmlNode* node, const char* tag) {
	vector<xmlNode*> nodes;
	for (xmlNode* cur = node->children; cur; cur = cur->next) {
		if (namecmp(cur->name, tag))
			nodes.push_back(cur);
		if (vector<xmlNode*> ret = findElements(cur, tag); !ret.empty())
			nodes.insert(nodes.end(), ret.begin(), ret.end());
	}
	return nodes;
}

xmlNode* WebSource::findElement(xmlNode* node, const char* tag, const char* attr, const char* val) {
	for (xmlNode* cur = node->children; cur; cur = cur->next) {
		if (namecmp(cur->name, tag) && hasAttr(cur, attr, val))
			return cur;
		if (xmlNode* ret = findElement(cur, tag, attr, val))
			return ret;
	}
	return nullptr;
}

vector<xmlNode*> WebSource::findElements(xmlNode* node, const char* tag, const char* attr, const char* val) {
	vector<xmlNode*> nodes;
	for (xmlNode* cur = node->children; cur; cur = cur->next) {
		if (namecmp(cur->name, tag) && hasAttr(cur, attr, val))
			nodes.push_back(cur);
		if (vector<xmlNode*> ret = findElements(cur, tag, attr, val); !ret.empty())
			nodes.insert(nodes.end(), ret.begin(), ret.end());
	}
	return nodes;
}

string WebSource::getContent(xmlNode* node) {
	string text;
	for (xmlNode* cur = node->children; cur; cur = cur->next)
		if (cur->type == XML_TEXT_NODE)
			text += reinterpret_cast<char*>(cur->content);
	return text;
}

string WebSource::getAttr(xmlNode* node, const char* attr) {
	if (xmlChar* prop = xmlGetProp(node, reinterpret_cast<const xmlChar*>(attr))) {
		string ret = reinterpret_cast<char*>(prop);
		xmlFree(prop);
		return ret;
	}
	return string();
}

bool WebSource::hasAttr(xmlNode* node, const char* attr, const char* val) {
	if (xmlChar* prop = xmlGetProp(node, reinterpret_cast<const xmlChar*>(attr))) {
		bool ret = !strcmp(reinterpret_cast<char*>(prop), val);
		xmlFree(prop);
		return ret;
	}
	return false;
}

// MANGAHERE

WebSource::Type Mangahere::source() const {
	return MANGAHERE;
}

vector<pairStr> Mangahere::query(const string& text) {
	vector<pairStr> results;
	for (string link = string(baseUrl()) + "/search?title=" + toUrl(text); !link.empty();) {
		xmlDoc* doc = downloadHtml(link);
		xmlNode* root = xmlDocGetRootElement(doc);
		if (!root)
			break;

		for (xmlNode* node : findElements(root, "p", "class", "manga-list-4-item-title"))
			if (node = findElement(node, "a"))
				if (string lnk = getAttr(node, "href"); !link.empty())
					results.emplace_back(trim(getContent(node)), baseUrl() + lnk);

		link.clear();
		if (xmlNode* next = findElement(root, "div", "class", "pager-list-left"))
			if (vector<xmlNode*> pgs = findElements(next, "a"); !pgs.empty())
				if (string page = getAttr(pgs.back(), "href"); !page.empty() && page != "javascript:void(0)")
					link = baseUrl() + page;
		xmlFreeDoc(doc);
	}
	return results;
}

vector<pairStr> Mangahere::getChapters(const string& url) {
	vector<pairStr> chapters;
	xmlDoc* doc = downloadHtml(url);
	xmlNode* root = xmlDocGetRootElement(doc);
	if (!root)
		return chapters;

	if (xmlNode* chaps = findElement(root, "ul", "class", "detail-main-list"))
		for (xmlNode* it : findElements(chaps, "a"))
			if (string name, link = getAttr(it, "href"); !link.empty()) {
				if (xmlNode* node = findElement(it, "p", "class", "title3"))
					name = trim(getContent(node));
				chapters.emplace_back(std::move(name), baseUrl() + link);
			}
	xmlFreeDoc(doc);
	return chapters;
}

DownloadState Mangahere::downloadPictures(const string& url, const fs::path& drc) {
	for (string link = url; !link.empty();) {
		xmlDoc* doc = downloadHtml(link);
		xmlNode* root = xmlDocGetRootElement(doc);
		if (!root)
			break;

		if (World::downloader()->getDlState() != DownloadState::run) {
			xmlFreeDoc(doc);
			break;
		}
		if (xmlNode* pic = findElement(root, "img", "class", "reader-main-img"))
			if (string ploc = getAttr(pic, "src"); !ploc.empty())
				if (downloadFile("https:" + ploc, drc) != DownloadState::run) {
					xmlFreeDoc(doc);
					break;
				}

		link.clear();
		if (xmlNode* next = findElement(root, "div", "class", "pager-list-left"))
			if (next = findElement(next, "span"))
				if (vector<xmlNode*> pgs = findElements(next, "a"); !pgs.empty())
					if (string page = getAttr(pgs.back(), "href"); !page.empty() && page != "javascript:void(0)")
						link = baseUrl() + link;
		xmlFreeDoc(doc);
	}
	return World::downloader()->getDlState();
}

// MANGAHERE

WebSource::Type Mangamaster::source() const {
	return MANGAMASTER;
}

vector<pairStr> Mangamaster::query(const string& text) {
	vector<pairStr> results;
	for (string link = string(baseUrl()) + "/comics?q[name_cont]=" + toUrl(text); !link.empty();) {
		xmlDoc* doc = downloadHtml(link);
		xmlNode* root = xmlDocGetRootElement(doc);
		if (!root)
			break;

		for (xmlNode* node : findElements(root, "a", "class", "comic-item__name"))
			if (string lnk = getAttr(node, "href"); !link.empty())
				results.emplace_back(trim(getContent(node)), baseUrl() + lnk);

		link.clear();
		if (xmlNode* next = findElement(root, "div", "class", "comics-navigate-link comics-navigate-link-next"))
			if (string page = getAttr(next, "href"); !page.empty())
				link = baseUrl() + page;
		xmlFreeDoc(doc);
	}
	return results;
}

vector<pairStr> Mangamaster::getChapters(const string& url) {
	vector<pairStr> chapters;
	xmlDoc* doc = downloadHtml(url);
	xmlNode* root = xmlDocGetRootElement(doc);
	if (!root)
		return chapters;

	if (xmlNode* chaps = findElement(root, "div", "class", "comic-chapters"))
		for (xmlNode* it : findElements(chaps, "a", "class", "comic-chapter"))
			if (string name = trim(getContent(it)), link = getAttr(it, "href"); !link.empty())
				chapters.emplace_back(std::move(name), baseUrl() + link);
	xmlFreeDoc(doc);
	return chapters;
}

DownloadState Mangamaster::downloadPictures(const string& url, const fs::path& drc) {
	xmlDoc* doc = downloadHtml(url);
	xmlNode* root = xmlDocGetRootElement(doc);
	if (!root)
		return World::downloader()->getDlState();

	if (xmlNode* cont = findElement(root, "div", "class", "chapter-pages"))
		for (xmlNode* it : findElements(cont, "a", "class", "chapter-page__img")) {
			if (World::downloader()->getDlState() != DownloadState::run)
				break;
			if (string ploc = getAttr(it, "src"); !ploc.empty())
				if (downloadFile(ploc, drc) != DownloadState::run)
					break;
		}
	xmlFreeDoc(doc);
	return World::downloader()->getDlState();
}

// NHENTAI

WebSource::Type Nhentai::source() const {
	return NHENTAI;
}

vector<pairStr> Nhentai::query(const string& text) {
	vector<pairStr> results;
	for (string link = string(baseUrl()) + "/search/?q=" + toUrl(text); !link.empty();) {
		xmlDoc* doc = downloadHtml(link);
		xmlNode* root = xmlDocGetRootElement(doc);
		if (!root)
			break;

		for (xmlNode* node : findElements(root, "a", "class", "cover"))
			if (string title, lnk = getAttr(node, "href"); !link.empty()) {
				if (node = findElement(node, "div", "class", "caption"))
					title = trim(getContent(node));
				results.emplace_back(std::move(title), baseUrl() + lnk);
			}

		link.clear();
		if (xmlNode* next = findElement(root, "a", "class", "next"))
			if (string page = getAttr(next, "href"); !page.empty())
				link = string(baseUrl()) + "/search/" + page;
		xmlFreeDoc(doc);
	}
	return results;
}

vector<pairStr> Nhentai::getChapters(const string& url) {
	return { pair("All", url) };
}

DownloadState Nhentai::downloadPictures(const string& url, const fs::path& drc) {
	xmlDoc* doc = downloadHtml(url);
	xmlNode* root = xmlDocGetRootElement(doc);
	if (!root)
		return World::downloader()->getDlState();

	for (xmlNode* thumb : findElements(root, "a", "class", "gallerythumb")) {
		if (World::downloader()->getDlState() != DownloadState::run) {
			xmlFreeDoc(doc);
			break;
		}
		if (string link = getAttr(thumb, "href"); !link.empty())
			if (string ploc = getPictureUrl(baseUrl() + link); !ploc.empty())
				if (downloadFile(ploc, drc) != DownloadState::run) {
					xmlFreeDoc(doc);
					break;
				}
	}
	xmlFreeDoc(doc);
	return World::downloader()->getDlState();
}

string Nhentai::getPictureUrl(const string& url) {
	string ploc;
	xmlDoc* doc = downloadHtml(url);
	xmlNode* root = xmlDocGetRootElement(doc);
	if (!root)
		return ploc;

	if (xmlNode* pic = findElement(root, "img", "class", "fit-horizontal"))
		ploc = getAttr(pic, "src");
	xmlFreeDoc(doc);
	return ploc;
}

// DOWNLOADER

Downloader::Downloader() :
	queueLock(SDL_CreateMutex()),
	dlProc(nullptr),
	dlState(DownloadState::stop)
{
	xmlInitParser();
	CURL* curlMain = curl_easy_init();
	if (!curlMain)
		throw std::runtime_error("failed to initialize curlMain");
	curl_easy_setopt(curlMain, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curlMain, CURLOPT_WRITEFUNCTION, writeText);

	CURL* curlFile = curl_easy_init();
	if (!curlFile)
		throw std::runtime_error("failed to initialize curlFile");
	curl_easy_setopt(curlFile, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(curlFile, CURLOPT_WRITEFUNCTION, nullptr);
	curl_easy_setopt(curlFile, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(curlFile, CURLOPT_XFERINFOFUNCTION, progress);
	curl_easy_setopt(curlFile, CURLOPT_XFERINFODATA, this);
	source = std::make_unique<Mangahere>(curlMain, SDL_CreateMutex(), curlFile);
}

Downloader::~Downloader() {
	curl_easy_cleanup(source->curlFile);
	curl_easy_cleanup(source->curlMain);
	SDL_DestroyMutex(source->mainLock);
	SDL_DestroyMutex(queueLock);
	xmlCleanupParser();
}

void Downloader::setSource(WebSource::Type type) {
	switch (type) {
	case WebSource::MANGAHERE:
		source = std::make_unique<Mangahere>(source->curlMain, source->mainLock, source->curlFile);
		break;
	case WebSource::MANGAMASTER:
		source = std::make_unique<Mangamaster>(source->curlMain, source->mainLock, source->curlFile);
		break;
	case WebSource::NHENTAI:
		source = std::make_unique<Nhentai>(source->curlMain, source->mainLock, source->curlFile);
	}
}

sizet Downloader::writeText(char* ptr, sizet size, sizet nmemb, void* userdata) {
	sizet len = size * nmemb;
	static_cast<string*>(userdata)->append(ptr, len);
	return len;
}

int Downloader::progress(void* clientp, cofft, cofft, cofft, cofft) {
	return static_cast<Downloader*>(clientp)->dlState != DownloadState::run;
}

bool Downloader::downloadComic(const Comic& info) {
	while (SDL_TryLockMutex(queueLock));
	dlQueue.push_back(info);
	SDL_UnlockMutex(queueLock);
	return startProc();
}

bool Downloader::startProc() {
	if (dlState == DownloadState::stop && !dlQueue.empty()) {
		dlState = DownloadState::run;
		if (!(dlProc = SDL_CreateThread(downloadChaptersThread, "", this)))
			dlState = DownloadState::stop;
	}
	return dlState == DownloadState::run;
}

void Downloader::interruptProc() {
	dlState = DownloadState::stop;
	finishProc();
}

void Downloader::finishProc() {
	SDL_WaitThread(dlProc, nullptr);
	dlProc = nullptr;
}

void Downloader::deleteEntry(sizet id) {
	while (SDL_TryLockMutex(queueLock));
	if (id < dlQueue.size()) {
		if (id)
			dlQueue.erase(dlQueue.begin() + pdift(id));
		else
			dlState = DownloadState::skip;
	}
	SDL_UnlockMutex(queueLock);
}

void Downloader::clearQueue() {
	interruptProc();
	dlQueue.clear();
}

int Downloader::downloadChaptersThread(void* data) {
	Downloader* dwn = static_cast<Downloader*>(data);
	while (SDL_TryLockMutex(dwn->queueLock));	// lock for first iteration

	while (!dwn->dlQueue.empty()) {	// iterate over comics in queue
		dwn->dlProg = mvec2(0);
		pushEvent(UserCode::downloadNext);

		if (fs::path bdrc = World::sets()->getDirLib() / FileSys::validateFilename(fs::u8path(dwn->dlQueue.front().title)); fs::is_directory(bdrc) || fs::create_directories(bdrc)) {	// create comic base directory in library
			vector<pairStr> chaps = dwn->dlQueue.front().chapters;
			SDL_UnlockMutex(dwn->queueLock);	// unlock after iteration check and copying chapter data

			switch (dwn->downloadChapters(chaps, bdrc)) {
			case DownloadState::stop:
				pushEvent(UserCode::downlaodFinished);
				return 1;
			case DownloadState::skip:
				dwn->dlState = DownloadState::run;
			}
			while (SDL_TryLockMutex(dwn->queueLock));	// lock for next iteration
		}
		dwn->dlQueue.pop_front();
	}
	SDL_UnlockMutex(dwn->queueLock);	// unlock after last dlPos check
	dwn->dlState = DownloadState::stop;
	pushEvent(UserCode::downlaodFinished);
	return 0;
}

DownloadState Downloader::downloadChapters(vector<pairStr> chaps, const fs::path& bdrc) {
	dlProg = mvec2(0, chaps.size());
	for (sizet i = 0; i < chaps.size(); i++) {	// iterate over chapters in currently selected comic
		dlProg.x = i;
		pushEvent(UserCode::downloadProgress);

		if (fs::path cdrc = bdrc / FileSys::validateFilename(fs::u8path(chaps[i].first)); fs::is_directory(cdrc) || fs::create_directories(cdrc))	// create chapter directory
			if (DownloadState state = source->downloadPictures(chaps[i].second, cdrc); state != DownloadState::run)	// download pictures of current chapter
				return state;
	}
	return dlState;
}
#endif
