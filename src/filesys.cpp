/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "filesys.h"
#include "util/string.h"
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <fstream>
#include "log.h"
#include "config.h"
#include "porting.h"

namespace fs
{

/*********
 * POSIX *
 *********/

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

std::vector<DirListNode> GetDirListing(const std::string &pathstring)
{
	std::vector<DirListNode> listing;

	DIR *dp;
	struct dirent *dirp;
	if((dp = opendir(pathstring.c_str())) == NULL) {
		//infostream<<"Error("<<errno<<") opening "<<pathstring<<std::endl;
		return listing;
	}

	while ((dirp = readdir(dp)) != NULL) {
		// NOTE:
		// Be very sure to not include '..' in the results, it will
		// result in an epic failure when deleting stuff.
		if(strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0)
			continue;

		DirListNode node;
		node.name = dirp->d_name;

		int isdir = -1; // -1 means unknown

		/*
			POSIX doesn't define d_type member of struct dirent and
			certain filesystems on glibc/Linux will only return
			DT_UNKNOWN for the d_type member.

			Also we don't know whether symlinks are directories or not.
		*/
#ifdef _DIRENT_HAVE_D_TYPE
		if(dirp->d_type != DT_UNKNOWN && dirp->d_type != DT_LNK)
			isdir = (dirp->d_type == DT_DIR);
#endif /* _DIRENT_HAVE_D_TYPE */

		/*
			Was d_type DT_UNKNOWN, DT_LNK or nonexistent?
			If so, try stat().
		*/
		if(isdir == -1) {
			struct stat statbuf{};
			if (stat((pathstring + "/" + node.name).c_str(), &statbuf))
				continue;
			isdir = ((statbuf.st_mode & S_IFDIR) == S_IFDIR);
		}
		node.dir = isdir;
		listing.push_back(node);
	}
	closedir(dp);

	return listing;
}

bool CreateDir(const std::string &path)
{
	int r = mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	if (r == 0) {
		return true;
	}

	// If already exists, return true
	if (errno == EEXIST)
		return true;
	return false;

}

bool PathExists(const std::string &path)
{
	struct stat st{};
	return (stat(path.c_str(),&st) == 0);
}

bool IsPathAbsolute(const std::string &path)
{
	return path[0] == '/';
}

bool IsDir(const std::string &path)
{
	struct stat statbuf{};
	if(stat(path.c_str(), &statbuf))
		return false; // Actually error; but certainly not a directory
	return ((statbuf.st_mode & S_IFDIR) == S_IFDIR);
}

bool IsExecutable(const std::string &path)
{
	return access(path.c_str(), X_OK) == 0;
}

bool IsDirDelimiter(char c)
{
	return c == '/';
}

bool RecursiveDelete(const std::string &path)
{
	/*
		Execute the 'rm' command directly, by fork() and execve()
	*/

	infostream<<"Removing \""<<path<<"\""<<std::endl;

	pid_t child_pid = fork();

	if(child_pid == 0)
	{
		// Child
		const char *argv[4] = {
			"/bin/rm",
			"-rf",
			path.c_str(),
			NULL
		};

		verbosestream<<"Executing '"<<argv[0]<<"' '"<<argv[1]<<"' '"
				<<argv[2]<<"'"<<std::endl;

		execv(argv[0], const_cast<char**>(argv));

		// Execv shouldn't return. Failed.
		_exit(1);
	}
	else
	{
		// Parent
		int child_status;
		pid_t tpid;
		do{
			tpid = wait(&child_status);
		}while(tpid != child_pid);
		return (child_status == 0);
	}
}

bool DeleteSingleFileOrEmptyDirectory(const std::string &path)
{
	if (IsDir(path)) {
		bool did = (rmdir(path.c_str()) == 0);
		if (!did)
			errorstream << "rmdir errno: " << errno << ": " << strerror(errno)
					<< std::endl;
		return did;
	}

	bool did = (unlink(path.c_str()) == 0);
	if (!did)
		errorstream << "unlink errno: " << errno << ": " << strerror(errno)
				<< std::endl;
	return did;
}

std::string TempPath()
{
	/*
		Should the environment variables TMPDIR, TMP and TEMP
		and the macro P_tmpdir (if defined by stdio.h) be checked
		before falling back on /tmp?

		Probably not, because this function is intended to be
		compatible with lua's os.tmpname which under the default
		configuration hardcodes mkstemp("/tmp/lua_XXXXXX").
	*/

	return DIR_DELIM "tmp";
}

std::string CreateTempFile()
{
	std::string path = TempPath() + DIR_DELIM "MT_XXXXXX";
	int fd = mkstemp(&path[0]); // modifies path
	if (fd == -1)
		return "";
	close(fd);
	return path;
}

/****************************
 * portable implementations *
 ****************************/

void GetRecursiveDirs(std::vector<std::string> &dirs, const std::string &dir)
{
	static const std::set<char> chars_to_ignore = { '_', '.' };
	if (dir.empty() || !IsDir(dir))
		return;
	dirs.push_back(dir);
	fs::GetRecursiveSubPaths(dir, dirs, false, chars_to_ignore);
}

std::vector<std::string> GetRecursiveDirs(const std::string &dir)
{
	std::vector<std::string> result;
	GetRecursiveDirs(result, dir);
	return result;
}

void GetRecursiveSubPaths(const std::string &path,
		  std::vector<std::string> &dst,
		  bool list_files,
		  const std::set<char> &ignore)
{
	std::vector<DirListNode> content = GetDirListing(path);
	for (const auto &n : content) {
		std::string fullpath = path + DIR_DELIM + n.name;
		if (ignore.count(n.name[0]))
			continue;
		if (list_files || n.dir)
			dst.push_back(fullpath);
		if (n.dir)
			GetRecursiveSubPaths(fullpath, dst, list_files, ignore);
	}
}

bool RecursiveDeleteContent(const std::string &path)
{
	infostream<<"Removing content of \""<<path<<"\""<<std::endl;
	std::vector<DirListNode> list = GetDirListing(path);
	for (const DirListNode &dln : list) {
		if(trim(dln.name) == "." || trim(dln.name) == "..")
			continue;
		std::string childpath = path + DIR_DELIM + dln.name;
		bool r = RecursiveDelete(childpath);
		if(!r) {
			errorstream << "Removing \"" << childpath << "\" failed" << std::endl;
			return false;
		}
	}
	return true;
}

bool CreateAllDirs(const std::string &path)
{

	std::vector<std::string> tocreate;
	std::string basepath = path;
	while(!PathExists(basepath))
	{
		tocreate.push_back(basepath);
		basepath = RemoveLastPathComponent(basepath);
		if(basepath.empty())
			break;
	}
	for(int i=tocreate.size()-1;i>=0;i--)
		if(!CreateDir(tocreate[i]))
			return false;
	return true;
}

bool CopyFileContents(const std::string &source, const std::string &target)
{
	FILE *sourcefile = fopen(source.c_str(), "rb");
	if(sourcefile == NULL){
		errorstream<<source<<": can't open for reading: "
			<<strerror(errno)<<std::endl;
		return false;
	}

	FILE *targetfile = fopen(target.c_str(), "wb");
	if(targetfile == NULL){
		errorstream<<target<<": can't open for writing: "
			<<strerror(errno)<<std::endl;
		fclose(sourcefile);
		return false;
	}

	size_t total = 0;
	bool retval = true;
	bool done = false;
	char readbuffer[BUFSIZ];
	while(!done){
		size_t readbytes = fread(readbuffer, 1,
				sizeof(readbuffer), sourcefile);
		total += readbytes;
		if(ferror(sourcefile)){
			errorstream<<source<<": IO error: "
				<<strerror(errno)<<std::endl;
			retval = false;
			done = true;
		}
		if(readbytes > 0){
			fwrite(readbuffer, 1, readbytes, targetfile);
		}
		if(feof(sourcefile) || ferror(sourcefile)){
			// flush destination file to catch write errors
			// (e.g. disk full)
			fflush(targetfile);
			done = true;
		}
		if(ferror(targetfile)){
			errorstream<<target<<": IO error: "
					<<strerror(errno)<<std::endl;
			retval = false;
			done = true;
		}
	}
	infostream<<"copied "<<total<<" bytes from "
		<<source<<" to "<<target<<std::endl;
	fclose(sourcefile);
	fclose(targetfile);
	return retval;
}

bool CopyDir(const std::string &source, const std::string &target)
{
	if(PathExists(source)){
		if(!PathExists(target)){
			fs::CreateAllDirs(target);
		}
		bool retval = true;
		std::vector<DirListNode> content = fs::GetDirListing(source);

		for (const auto &dln : content) {
			std::string sourcechild = source + DIR_DELIM + dln.name;
			std::string targetchild = target + DIR_DELIM + dln.name;
			if(dln.dir){
				if(!fs::CopyDir(sourcechild, targetchild)){
					retval = false;
				}
			}
			else {
				if(!fs::CopyFileContents(sourcechild, targetchild)){
					retval = false;
				}
			}
		}
		return retval;
	}

	return false;
}

bool MoveDir(const std::string &source, const std::string &target)
{
	infostream << "Moving \"" << source << "\" to \"" << target << "\"" << std::endl;

	// If target exists as empty folder delete, otherwise error
	if (fs::PathExists(target)) {
		if (rmdir(target.c_str()) != 0) {
			errorstream << "MoveDir: target \"" << target
				<< "\" exists as file or non-empty folder" << std::endl;
			return false;
		}
	}

	// Try renaming first which is instant
	if (fs::Rename(source, target))
		return true;

	infostream << "MoveDir: rename not possible, will copy instead" << std::endl;
	bool retval = fs::CopyDir(source, target);
	if (retval)
		retval &= fs::RecursiveDelete(source);
	return retval;
}

bool PathStartsWith(const std::string &path, const std::string &prefix)
{
	size_t pathsize = path.size();
	size_t pathpos = 0;
	size_t prefixsize = prefix.size();
	size_t prefixpos = 0;
	for(;;){
		bool delim1 = pathpos == pathsize
			|| IsDirDelimiter(path[pathpos]);
		bool delim2 = prefixpos == prefixsize
			|| IsDirDelimiter(prefix[prefixpos]);

		if(delim1 != delim2)
			return false;

		if(delim1){
			while(pathpos < pathsize &&
					IsDirDelimiter(path[pathpos]))
				++pathpos;
			while(prefixpos < prefixsize &&
					IsDirDelimiter(prefix[prefixpos]))
				++prefixpos;
			if(prefixpos == prefixsize)
				return true;
			if(pathpos == pathsize)
				return false;
		}
		else{
			size_t len = 0;
			do{
				char pathchar = path[pathpos+len];
				char prefixchar = prefix[prefixpos+len];
				if(FILESYS_CASE_INSENSITIVE){
					pathchar = tolower(pathchar);
					prefixchar = tolower(prefixchar);
				}
				if(pathchar != prefixchar)
					return false;
				++len;
			} while(pathpos+len < pathsize
					&& !IsDirDelimiter(path[pathpos+len])
					&& prefixpos+len < prefixsize
					&& !IsDirDelimiter(
						prefix[prefixpos+len]));
			pathpos += len;
			prefixpos += len;
		}
	}
}

std::string RemoveLastPathComponent(const std::string &path,
		std::string *removed, int count)
{
	if(removed)
		removed->clear();

	size_t remaining = path.size();

	for(int i = 0; i < count; ++i){
		// strip a dir delimiter
		while(remaining != 0 && IsDirDelimiter(path[remaining-1]))
			remaining--;
		// strip a path component
		size_t component_end = remaining;
		while(remaining != 0 && !IsDirDelimiter(path[remaining-1]))
			remaining--;
		size_t component_start = remaining;
		// strip a dir delimiter
		while(remaining != 0 && IsDirDelimiter(path[remaining-1]))
			remaining--;
		if(removed){
			std::string component = path.substr(component_start,
					component_end - component_start);
			if(i)
				*removed = component + DIR_DELIM + *removed;
			else
				*removed = component;
		}
	}
	return path.substr(0, remaining);
}

std::string RemoveRelativePathComponents(std::string path)
{
	size_t pos = path.size();
	size_t dotdot_count = 0;
	while (pos != 0) {
		size_t component_with_delim_end = pos;
		// skip a dir delimiter
		while (pos != 0 && IsDirDelimiter(path[pos-1]))
			pos--;
		// strip a path component
		size_t component_end = pos;
		while (pos != 0 && !IsDirDelimiter(path[pos-1]))
			pos--;
		size_t component_start = pos;

		std::string component = path.substr(component_start,
				component_end - component_start);
		bool remove_this_component = false;
		if (component == ".") {
			remove_this_component = true;
		} else if (component == "..") {
			remove_this_component = true;
			dotdot_count += 1;
		} else if (dotdot_count != 0) {
			remove_this_component = true;
			dotdot_count -= 1;
		}

		if (remove_this_component) {
			while (pos != 0 && IsDirDelimiter(path[pos-1]))
				pos--;
			if (component_start == 0) {
				// We need to remove the delemiter too
				path = path.substr(component_with_delim_end, std::string::npos);
			} else {
				path = path.substr(0, pos) + DIR_DELIM +
					path.substr(component_with_delim_end, std::string::npos);
			}
			if (pos > 0)
				pos++;
		}
	}

	if (dotdot_count > 0)
		return "";

	// remove trailing dir delimiters
	pos = path.size();
	while (pos != 0 && IsDirDelimiter(path[pos-1]))
		pos--;
	return path.substr(0, pos);
}

std::string AbsolutePath(const std::string &path)
{
	char *abs_path = realpath(path.c_str(), NULL);
	if (!abs_path) return "";
	std::string abs_path_str(abs_path);
	free(abs_path);
	return abs_path_str;
}

const char *GetFilenameFromPath(const char *path)
{
	const char *filename = strrchr(path, DIR_DELIM_CHAR);
	// Consistent with IsDirDelimiter this function handles '/' too
	if (DIR_DELIM_CHAR != '/') {
		const char *tmp = strrchr(path, '/');
		if (tmp && tmp > filename)
			filename = tmp;
	}
	return filename ? filename + 1 : path;
}

bool safeWriteToFile(const std::string &path, const std::string &content)
{
	std::string tmp_file = path + ".~mt";

	// Write to a tmp file
	bool tmp_success = false;

	std::ofstream os(tmp_file.c_str(), std::ios::binary);
	if (!os.good()) {
		return false;
	}
	os << content;
	os.flush();
	os.close();
	tmp_success = !os.fail();

	if (!tmp_success) {
		remove(tmp_file.c_str());
		return false;
	}

	bool rename_success = false;

	// Move the finished temporary file over the real file
	// On POSIX compliant systems rename() is specified to be able to swap the
	// file in place of the destination file, making this a truly error-proof
	// transaction.
	rename_success = rename(tmp_file.c_str(), path.c_str()) == 0;
	if (!rename_success) {
		warningstream << "Failed to write to file: " << path.c_str() << std::endl;
		// Remove the temporary file because moving it over the target file
		// failed.
		remove(tmp_file.c_str());
		return false;
	}

	return true;
}

bool ReadFile(const std::string &path, std::string &out)
{
	std::ifstream is(path, std::ios::binary | std::ios::ate);
	if (!is.good()) {
		return false;
	}

	auto size = is.tellg();
	out.resize(size);
	is.seekg(0);
	is.read(&out[0], size);

	return !is.fail();
}

bool Rename(const std::string &from, const std::string &to)
{
	return rename(from.c_str(), to.c_str()) == 0;
}

} // namespace fs
