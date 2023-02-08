#pragma once

#ifdef DOWNLOADER
#include "utils/utils.h"
#include <deque>
#include <mutex>
#include <thread>

using std::deque;
using cofft = llong;	// curl_off_t
using CURL = void;
using xmlChar = uchar;
struct _xmlDoc;
using xmlDoc = _xmlDoc;
struct _xmlNode;
using xmlNode = _xmlNode;

enum class DownloadState : uint8 {
	run,
	stop,
	skip
};

struct Comic {
	string title;
	vector<pair<string, string>> chapters;	// name, url

	Comic(string capt = string(), vector<pair<string, string>>&& chaps = vector<pair<string, string>>());
};

class WebSource {
public:
	enum Type : uint8 {
		mangahere,
		mangamaster,
		nhentai
	};
	static constexpr array<const char*, nhentai + 1> sourceNames = {
		"MangaHere",
		"MangaMaster",
		"nhentai"
	};
	static constexpr array<const char*, nhentai + 1> sourceUrls = {
		"https://www.mangahere.cc",
		"http://www.mangamaster.net",
		"https://nhentai.net"
	};

protected:
	CURL* curlMain;			// for downloading html
	std::mutex mainLock;	// lock before using curlMain
	CURL* curlFile;			// handle for downloading files in dlQueue

public:
	WebSource(CURL* curlm, CURL* cfile);
	virtual ~WebSource() = default;

	const char* name() const;
	const char* baseUrl() const;
	virtual Type source() const;	// dummy function

	virtual vector<pair<string, string>> query(const string& text) = 0;
	virtual vector<pair<string, string>> getChapters(const string& url) = 0;	// url needs to be of the comic's main info page; returned chapter lit has pairs of first name, second url
	virtual DownloadState downloadPictures(const string& url, const fs::path& drc) = 0;	// returns non-zero if interrupted

protected:
	xmlDoc* downloadHtml(const string& url);
	DownloadState downloadFile(const string& url, const fs::path& drc);	// returns non-zero if progress got interrupted
	string toUrl(string_view str);

	static xmlNode* findElement(xmlNode* node, const char* tag);
	static vector<xmlNode*> findElements(xmlNode* node, const char* tag);
	static xmlNode* findElement(xmlNode* node, const char* tag, const char* attr, const char* val);
	static vector<xmlNode*> findElements(xmlNode* node, const char* tag, const char* attr, const char* val);
	static string getContent(xmlNode* node);
	static string getAttr(xmlNode* node, const char* attr);
	static bool hasAttr(xmlNode* node, const char* attr, const char* val);
	static bool namecmp(const xmlChar* name, const char* str);

	friend class Downloader;
};

inline const char* WebSource::name() const {
	return sourceNames[source()];
}

inline const char* WebSource::baseUrl() const {
	return sourceUrls[source()];
}

inline bool WebSource::namecmp(const xmlChar* name, const char* str) {
	return name && !strcmp(reinterpret_cast<const char*>(name), str);
}

class Mangahere : public WebSource {
public:
	using WebSource::WebSource;
	~Mangahere() final = default;

	Type source() const final;
	vector<pair<string, string>> query(const string& text) final;
	vector<pair<string, string>> getChapters(const string& url) final;
	DownloadState downloadPictures(const string& url, const fs::path& drc) final;
};

class Mangamaster : public WebSource {
public:
	using WebSource::WebSource;
	~Mangamaster() final = default;

	Type source() const final;
	vector<pair<string, string>> query(const string& text) final;
	vector<pair<string, string>> getChapters(const string& url) final;
	DownloadState downloadPictures(const string& url, const fs::path& drc) final;
};

class Nhentai : public WebSource {
public:
	using WebSource::WebSource;
	~Nhentai() final = default;

	Type source() const final;
	vector<pair<string, string>> query(const string& text) final;
	vector<pair<string, string>> getChapters(const string& url) final;
	DownloadState downloadPictures(const string& url, const fs::path& drc) final;

private:
	string getPictureUrl(const string& url);
};

class Downloader {
public:
	std::mutex queueLock;	// lock when using dlQueue
private:
	uptr<WebSource> source;	// for downloading and interpreting htmls
	std::thread dlProc;		// thread for downloading files in dlQueue
	deque<Comic> dlQueue;	// current downloads
	mvec2 dlProg;			// download progress of current comic
	DownloadState dlState = DownloadState::stop;	// for controlling dlProc

public:
	Downloader();
	~Downloader();

	WebSource* getSource() const;
	void setSource(WebSource::Type type);
	const deque<Comic>& getDlQueue() const;
	DownloadState getDlState() const;
	const mvec2& getDlProg() const;
	bool downloadComic(const Comic& info);	// info.chapters needs to be filled with names and urls for getPictures
	bool startProc();
	void interruptProc();
	void finishProc();
	void deleteEntry(size_t id);
	void clearQueue();

private:
	static size_t writeText(char* ptr, size_t size, size_t nmemb, void* userdata);
	static int progress(void* clientp, cofft dltotal, cofft dlnow, cofft ultotal, cofft ulnow);
	int downloadChaptersThread();
	DownloadState downloadChapters(const vector<pair<string, string>>& chaps, const fs::path& bdrc);	// returns non-zero if interrupted
};

inline WebSource* Downloader::getSource() const {
	return source.get();
}

inline const deque<Comic>& Downloader::getDlQueue() const {
	return dlQueue;
}

inline DownloadState Downloader::getDlState() const {
	return dlState;
}

inline const mvec2& Downloader::getDlProg() const {
	return dlProg;
}

inline void Downloader::finishProc() {
	dlProc.join();
}
#endif
