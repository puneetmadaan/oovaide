/*
 * oovEdit.h
 *
 *  Created on: Feb 16, 2014
 *  \copyright 2014 DCBlaha.  Distributed under the GPL.
 */

#ifndef OOVEDIT_H_
#define OOVEDIT_H_

#include "EditFiles.h"
#include "EditOptions.h"
#include "OovProcess.h"

class Editor:public DebuggerListener
    {
    public:
	Editor();
	void init();
	void loadSettings();
	void saveSettings();
	static gboolean onIdle(gpointer data);
	void openTextFile(char const * const fn);
	void openTextFile();
	bool saveTextFile();
	bool saveAsTextFileWithDialog();
	void findDialog();
	void findAgain(bool forward);
        void findInFilesDialog();
        void findInFiles(char const * const srchStr, char const * const path,
        	bool caseSensitive, GtkTextView *view);
//	void setTabs(int numSpaces);
	void setStyle();
	void cut()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->cut();
	    }
	void copy()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->copy();
	    }
	void paste()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->paste();
	    }
	void deleteSel()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->deleteSel();
	    }
	void undo()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->undo();
	    }
	void redo()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->redo();
	    }
	bool checkExitSave()
	    {
	    return mEditFiles.checkExitSave();
	    }
	void gotoToken(eFindTokenTypes ft)
	    {
	    if(mEditFiles.getEditView())
		{
		std::string fn;
		int offset;
		if(mEditFiles.getEditView()->find(ft, fn, offset))
		    {
		    mEditFiles.viewFile(fn.c_str(), offset);
		    }
		}
	    }
	void gotoDeclaration()
	    {
	    gotoToken(FT_FindDecl);
	    }
	void gotoDefinition()
	    {
	    gotoToken(FT_FindDef);
	    }
	Builder &getBuilder()
	    { return mBuilder; }
	Debugger &getDebugger()
	    { return mDebugger; }
	EditFiles &getEditFiles()
	    { return mEditFiles; }
	void setProjectDir(char const * const projDir)
	    { mProjectDir = projDir; }
	void gotoLine(int lineNum)
	    { mEditFiles.gotoLine(lineNum); }
	void gotoFileLine(std::string const &lineBuf);
	void bufferInsertText(GtkTextBuffer *textbuffer, GtkTextIter *location,
	        gchar *text, gint len)
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->bufferInsertText(textbuffer, location, text, len);
	    }
	void bufferDeleteRange(GtkTextBuffer *textbuffer, GtkTextIter *start,
	        GtkTextIter *end)
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->bufferDeleteRange(textbuffer, start, end);
	    }
	void drawHighlight()
	    {
	    if(mEditFiles.getEditView())
		mEditFiles.getEditView()->drawHighlight();
	    }
	void editPreferences();
	void setPreferencesWorkingDir();
	void idleDebugStatusChange(Debugger::eChangeStatus st);
	virtual void DebugOutput(char const * const str)
	    {
	    mDebugOut.append(str);
	    }
	virtual void DebugStatusChanged()
	    {
	    }
	void debugSetStackFrame(char const * const frameLine);

    private:
	Builder mBuilder;
	EditFiles mEditFiles;
	Debugger mDebugger;
	std::string mLastSearch;
	std::string mProjectDir;
	bool mLastSearchCaseSensitive;
	std::string mDebugOut;
	EditOptions mEditOptions;
	GuiTree mVarView;
	void find(char const * const findStr, bool forward, bool caseSensitive);
	void findAndReplace(char const * const findStr, bool forward, bool caseSensitive,
		char const * const replaceStr);
	void setModuleName(const char *mn)
	    {
	    gtk_window_set_title(GTK_WINDOW(mBuilder.getWidget("MainWindow")), mn);
	    }
};


#endif /* OOVEDIT_H_ */
