/*
 * Exchange crawler
 *
 * June, 2007
 */

#include <sys/types.h>
#include <sys/time.h>
#include <string.h>


#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#include "webdav.h"
#include "../common/timediff.h"
#include "../logger/logger.h"

#define DEBUG

char *
make_userpass(const char *username, const char *password)
{
	char *buf;

	buf = malloc(strlen(username) + strlen(password) + 1 + 1);
	if (buf == NULL)
		return NULL;
	sprintf(buf, "%s:%s", username, password);

	return buf;
}

void
free_userpass(char *buf)
{
	free(buf);
}
 
int
ex_write_buffer(void *buffer, size_t size, size_t nmemb, void *stream)
{
	struct ex_buffer *buf;
	size_t origsize;

	if (size != 1)
		bblog(ERROR, "ex_write_buffer(): size not equal to one!");
	buf = stream;

	origsize = 0;
	if (buf->buf == NULL) {
		buf->buf = malloc(size * (nmemb + 1));
		buf->size = size * (nmemb + 1);
		buf->buf[0] = '\0';
	} else {
		origsize = buf->size;
		buf->size += (size * nmemb);
		buf->buf = realloc(buf->buf, buf->size);
	}
	if (buf->buf == NULL)
		return -1;

	/* XXX: Assuming size == sizeof(char) */
	//strncat(buf->buf, buffer, size * nmemb);
	strncpy(buf->buf + origsize - (origsize == 0 ? 0 : size), buffer, (size * nmemb)+1);
	buf->buf[buf->size-1] = '\0';
	return nmemb;
}

/* Free the returned value */
char *
ex_getEmail(const char *url, struct ex_buffer *buf, CURL ** curl)
{
	CURLcode result;
	struct curl_slist *headers = NULL;
	struct timeval start_time, end_time;

	bblog(INFO, "ex_getEmail(url=%s)",url);

	//curl = curl_easy_init();
	if (*curl == NULL)
		return NULL;

	buf->buf = NULL;
	headers = curl_slist_append(headers, "Translate: f");
	curl_easy_setopt(*curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(*curl, CURLOPT_URL, url);
	curl_easy_setopt(*curl, CURLOPT_CUSTOMREQUEST, "GET");
	curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, ex_write_buffer);
	curl_easy_setopt(*curl, CURLOPT_WRITEDATA, buf);
	gettimeofday(&start_time, NULL);
	result = curl_easy_perform(*curl);
	gettimeofday(&end_time, NULL);
	curl_slist_free_all(headers);
	bblog(DEBUGINFO, "Grab took: %f", getTimeDifference(&start_time,&end_time));

	return buf->buf;
}

void ex_logOff(CURL **curl) {

	curl_easy_cleanup(*curl);
}

CURL *ex_logOn(const char *mailboxurl, struct loginInfoFormat *login, char **errorm) {

	CURL *curl;
	CURLcode result;
	char *userpass;
	long code;
	char *redirecttarget;
	char *owaauth;
	char owaauthpath[PATH_MAX];
	*errorm = NULL;
	
	

	#ifdef DEBUG
	bblog(INFO, "ex_logOn(mailboxurl=%s, Exchangeurl=%s, username=%s, password=%s)", mailboxurl, login->Exchangeurl, login->username, login->password);
	#else
	bblog(INFO, "ex_logOn(mailboxurl=%s, Exchangeurl=%s, username=%s)", mailboxurl, login->Exchangeurl, login->username);
	#endif

	curl = curl_easy_init();
	if (curl == NULL) {
		asprintf(errorm,"Can't init curl_easy_init()");		
		return NULL;
	}

	// for debuging: Tar med headeren i http bodien
	//curl_easy_setopt(curl, CURLOPT_HEADER, 1);

	#ifdef DEBUG
    	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	#endif

	if (strstr(mailboxurl,"https://") != NULL) {
    		//tilater bugy serfikater.
    		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	//vi pr�ver f�rs � gj�re et vanlig kall til urlen for � se hva som skjer. 
    	curl_easy_setopt(curl, CURLOPT_URL, mailboxurl);

	result = curl_easy_perform(curl);
	result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

	#ifdef DEBUG
		//curl skriver av og til ut data uten \n p� slutten. 
		printf("\n");
	#endif

	bblog(DEBUGINFO, "code %i",code);

	//hvis vi fikke en redirect finner vi ut til hvor
	if (code == 302) {
    		result = curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL , &redirecttarget);
    		bblog(INFO, "res %d",result);
	    	bblog(INFO, "redirect's to \"%s\"",redirecttarget);
	}

	if (code == 302 && (strstr(redirecttarget,"exchweb/bin/auth/owalogon.asp") != NULL)) {

		bblog(INFO, "we have form based login!");

		char *destination;
		char *postData;

	    	/* First set the URL that is about to receive our POST. This URL can
	       	just as well be a https:// URL if that is what should receive the
	       	data. */
		snprintf(owaauthpath,sizeof(owaauthpath),"%s/exchweb/bin/auth/owaauth.dll",login->Exchangeurl);
	    	curl_easy_setopt(curl, CURLOPT_URL, owaauthpath);

	    	//bruke cookies
	    	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "-");

	    	//curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION , 0);

		//url enkoder destination feltet
		destination = curl_easy_escape(curl, mailboxurl, 0);

		//bygger port datane
		// skal v�re av slik: destination=https%3A%2F%2Fsbs.searchdaimon.com%2FExchange%2Feo&flags=0&username=eo&password=1234Asd&SubmitCreds=Log+On&trusted=0

		asprintf(&postData,"destination=%s&flags=0&username=%s&password=%s&SubmitCreds=Log+On&trusted=0",destination, login->username, login->password);

		bblog(DEBUGINFO, "postData new: %s",postData);

	    	/* Now specify the POST data */
	    	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);



	    	/* Perform the request, res will get the return code */
	    	result = curl_easy_perform(curl);

	    	//res = curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL , &redirecttarget);
	    	//printf("res %d\n");
	    	//printf("red to \"%s\"\n",redirecttarget);

	    	result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	    	bblog(INFO, "res %d, code %d",result,code);

		curl_free(destination);
		free(postData);
		
	} // aaaaaaaa:
	else if (code == 302 && (strstr(redirecttarget,"owa/auth/logon.aspx") != NULL)) {

		bblog(INFO, "we have form based login!");

		char *destination;
		char *postData;

	    	/* First set the URL that is about to receive our POST. This URL can
	       	just as well be a https:// URL if that is what should receive the
	       	data. */
		snprintf(owaauthpath,sizeof(owaauthpath),"%s/owa/auth/owaauth.dll",login->Exchangeurl);
	    	curl_easy_setopt(curl, CURLOPT_URL, owaauthpath);

	    	//bruke cookies
	    	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, "-");

	    	//curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION , 0);

		//url enkoder destination feltet
		destination = curl_easy_escape(curl, mailboxurl, 0);

		//bygger port datane
		// skal v�re av slik: destination=https%3A%2F%2Fsbs.searchdaimon.com%2FExchange%2Feo&flags=0&username=eo&password=1234Asd&SubmitCreds=Log+On&trusted=0

		asprintf(&postData,"destination=%s&flags=0&username=%s&password=%s&SubmitCreds=Log+On&trusted=0",destination, login->username, login->password);

		bblog(DEBUGINFO, "postData new: %s",postData);

	    	/* Now specify the POST data */
	    	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData);



	    	/* Perform the request, res will get the return code */
	    	result = curl_easy_perform(curl);

	    	//res = curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL , &redirecttarget);
	    	//printf("res %d\n");
	    	//printf("red to \"%s\"\n",redirecttarget);

	    	result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	    	bblog(INFO, "res %d, code %d",result,code);

		curl_free(destination);
		free(postData);
		
	} //aaaaaaaaaaaaa
	else if (code == 401) {
		if ((userpass = make_userpass(login->username, login->password)) == NULL)
			return NULL;

		bblog(INFO, "Got 401 error code. We have normal basic login.");

		curl_easy_setopt(curl, CURLOPT_USERPWD, userpass);

//		result = curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY); //allow all aut options. Include ntlm
		result = curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC); //allow all aut options. Include ntlm

		if (result != CURLE_OK) {
                        asprintf(errorm,"Can't set curl aut metod. Error code: %d.",result);
			return NULL;			
		} 

		//usikker om vi kan gj�re dette. Kan v�re at curl trenger datene siden.
		free_userpass(userpass);

		//etter at vi har satt bruker/pass pr�ver vi � logge p�.
    		curl_easy_setopt(curl, CURLOPT_URL, mailboxurl);
	
		result = curl_easy_perform(curl);
		result = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

		bblog(DEBUGINFO, "code %i",code);

		if (code == 302) {
                        curl_easy_getinfo(curl, CURLINFO_REDIRECT_URL , &redirecttarget);
                        bblog(INFO, "redirect to \"%s\"",redirecttarget);
		}

		if (code == 200) {

		}
		else if ( (code == 302) && (strstr(redirecttarget,"/owa/") != NULL) ) {
			//vi blir redirektet til owa. Noe som ser ut til � v�re helt normalt i Exchnage 2007.
		}
		else if (code == 302) {
                        asprintf(errorm,"Login 302 redirected to %s.",redirecttarget);
			return NULL;
		}
		else if (code == 401) {
			asprintf(errorm,"Wrong username or passord");
			return NULL;
		}
		else {
			asprintf(errorm,"Http error code %i",code);
//			return NULL;
		}

    	}
	else if ((code == 302) && (strstr(mailboxurl,"http://") != NULL) && (strstr(redirecttarget,"https://") != NULL)) {
		asprintf(errorm,"Got redirected to an httpS page. If your server only supports https please use an https url as the resource for Exchange.\n");		
		return NULL;
	}
	else if (code == 200) {
		/* Looks good */
	}
	else {
		asprintf(errorm,"Can't decide login type.");
		return NULL;		
	}


	bblog(INFO, "~ex_logOn()");

	return curl;
}

/* Free the returned value */
char *
ex_getContent(const char *url, CURL **curl, struct loginInfoFormat *login)
{
	CURLcode result;
	struct curl_slist *headers = NULL;
	struct ex_buffer buf;
	long code;

	#ifdef DEBUG
		//warn: skriver ut passord i klartekst.
		bblog(DEBUGINFO, "ex_getContent(url=%s)",url);
	#endif

	buf.buf = NULL;
	buf.size = 0;


	headers = curl_slist_append(headers, "Translate: f");
	headers = curl_slist_append(headers, "Content-type: text/xml");
	headers = curl_slist_append(headers, "Depth: 1");
	curl_easy_setopt(*curl, CURLOPT_HTTPHEADER, headers);
    
	curl_easy_setopt(*curl, CURLOPT_URL, url);
	curl_easy_setopt(*curl, CURLOPT_CUSTOMREQUEST, "PROPFIND");

	curl_easy_setopt(*curl, CURLOPT_WRITEFUNCTION, ex_write_buffer);
	curl_easy_setopt(*curl, CURLOPT_WRITEDATA, &buf);

	curl_easy_setopt(*curl, CURLOPT_POSTFIELDS,
		"<?xml version=\"1.0\"?>"
		"<d:propfind xmlns:d='DAV:' xmlns:c='urn:schemas:httpmail:' xmlns:m='urn:schemas:mailheader:' xmlns:p='http://schemas.microsoft.com/mapi/proptag/' xmlns:ex='http://schemas.microsoft.com/exchange/security/'>"
			"<d:prop>"
				"<d:displayname/>"
				"<d:getcontentlength/>"
				"<d:getlastmodified/>"
				"<p:xfff0102/>"
				"<ex:descriptor/>"
				"<p:x1A001E/>"
			"</d:prop>"
		"</d:propfind>"
	);

//http://schemas.microsoft.com/mapi/proptag/0x0E1D001E

	result = curl_easy_perform(*curl);
	result = curl_easy_getinfo(*curl, CURLINFO_RESPONSE_CODE, &code);
	curl_slist_free_all(headers);

	//printf("Mail\n\n%s\n\n", buf.buf);
	bblog(INFO, "response code %d",code);

	if (code == 401) {
		if (buf.buf != NULL) 
			free(buf.buf);

		return NULL;
	}
       //login timeout
       if (code == 440) {
               	char *errorm;
		ex_logOff(curl);
               	*curl = ex_logOn(url, login, &errorm);
		if (buf.buf != NULL) 
               		free(buf.buf);

               	return NULL;

        }

	#ifdef DEBUG
	bblog(DEBUGINFO, "buf.buf: \"%s\"",buf.buf);
	#endif

	return buf.buf;
}

