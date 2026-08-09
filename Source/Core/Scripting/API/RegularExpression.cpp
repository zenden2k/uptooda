/*

    Uptooda - free application for uploading images/files to the Internet

    Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

#include "RegularExpression.h"

#include "Core/Logging.h"
#include "../Squirrelnc.h"
#include "ScriptAPI.h"

using namespace pcrepp;

namespace ScriptAPI {

RegularExpression::RegularExpression(const std::string& expression, const std::string& flags)
{
    pcre_.reset( new Pcre(expression, flags));
}

RegularExpression::RegularExpression()
{

}
/*
RegularExpression::RegularExpression(const RegularExpression& r)
{

}
*/
Sqrat::Array RegularExpression::split(const std::string& piece)
{
    try {
        std::vector<std::string> res = pcre_->split(piece);
        Sqrat::Array obj(GetCurrentThreadVM(), res.size());
        for( size_t i = 0; i  < res.size(); i++ ) {
            obj.SetValue(i, res[i].c_str());
        }
        return obj;
    } catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return {};
    }
}

Sqrat::Array RegularExpression::splitWithLimitOffset(const std::string& piece, int limit, int start_offset)
{
    try {
        std::vector<std::string> res = pcre_->split(piece,limit,start_offset);
        Sqrat::Array obj(GetCurrentThreadVM(), res.size());
        for( size_t i = 0; i  < res.size(); i++ ) {
            obj.SetValue(i, res[i].c_str());
        }
        return obj;
    }
    catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return {};
    }
}

Sqrat::Array RegularExpression::splitWithLimit(const std::string& piece, int limit)
{
    try {
        std::vector<std::string> res = pcre_->split(piece,limit);
        Sqrat::Array obj(GetCurrentThreadVM(), res.size());
        for( size_t i = 0; i  < res.size(); i++ ) {
            obj.SetValue(i, res[i].c_str());
        }
        return obj;
    }
    catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return {};
    }
}
Sqrat::Array RegularExpression::splitWithLimitStartEndOffset(const std::string& piece, int limit, int start_offset, int end_offset)
{
    try {
        std::vector<std::string> res = pcre_->split(piece,limit,start_offset,end_offset);
        Sqrat::Array obj(GetCurrentThreadVM(), res.size());
        for( size_t i = 0; i  < res.size(); i++ ) {
            obj.SetValue(i, res[i].c_str());
        }
        return obj;
    }
    catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return {};
    }
}

std::string RegularExpression::getMatch(int pos) const
{
    try {
        return pcre_->get_match(pos);
    }
    catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return {};
    }
}

bool RegularExpression::search(const std::string& stuff)
{
    try {
        return pcre_->search(stuff);
    } catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return false;
    }
}

bool RegularExpression::searchWithOffset(const std::string& stuff, int OffSet)
{
    try {
        return pcre_->search(stuff, OffSet);
    } catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return false;
    }
}

std::string RegularExpression::replace(const std::string& piece, const std::string& with)
{
    try {
        return pcre_->replace(piece, with);
    } catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return {};
    }
}

int RegularExpression::getEntireMatchStart() const
{
    return pcre_->get_match_start();
}

int RegularExpression::getMatchStart(int pos) const
{
    try {
        return pcre_->get_match_start(pos);
    } catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return 0;
    }
}

int RegularExpression::getEntireMatchEnd() const
{
    return pcre_->get_match_end();
}

int RegularExpression::getMatchEnd(int pos) const
{
    try {
        return pcre_->get_match_end(pos);
    } catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return 0;
    }
}

bool RegularExpression::matched()
{
    return pcre_->matched();
}

int RegularExpression::matchesCount()
{
    return pcre_->matches();
}

Sqrat::Array RegularExpression::getSubStrings()
{
    try {
        std::vector<std::string>* substrings =  pcre_->get_sub_strings();
        if ( !substrings ) {
            return {};
        }
        Sqrat::Array res(GetCurrentThreadVM(), substrings->size());
        for ( size_t i = 0; i < substrings->size(); i++ ) {
            res.SetValue(i, (*substrings)[i].c_str());
        }
        return res;
    } catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return {};
    }
}

bool RegularExpression::match(const std::string& stuff)
{
    return search(stuff);
}

Sqrat::Array RegularExpression::findAll(const std::string& str)
{
    try {
        size_t pos = 0;
        Sqrat::Array res(GetCurrentThreadVM(), 0);
        while (pos <= str.length())
        {
            if ( pcre_->search(str, pos))
            {
                pos = pcre_->get_match_end()+1;
                int count = matchesCount();
                Sqrat::Array mat(GetCurrentThreadVM(), count);
                for ( int i = 0; i < count; i++ ) {
                    mat.SetValue(i, pcre_->get_match(i).c_str());
                }
                res.Append(mat);
            }
            else {
                break;
            }
        }
        return res;
    } catch( Pcre::exception& ex ) {
        LOG(ERROR) << ex.what();
        return {};
    }
}

ScriptAPI::RegularExpression CreateRegExp(const std::string& expression, const std::string& flags)
{
    return {expression,flags};
}

void RegisterRegularExpressionClass(Sqrat::SqratVM& vm) {
    using namespace Sqrat;
    Sqrat::RootTable& root = vm.GetRootTable();
    root.Bind("RegularExpression", Class<RegularExpression>(vm.GetVM(), "RegularExpression")
        .Func("search", &RegularExpression::search)
        .Func("findAll", &RegularExpression::findAll)
        .Func("match", &RegularExpression::match)
        .Func("searchWithOffset", &RegularExpression::searchWithOffset)
        .Func("getMatch", &RegularExpression::getMatch)
        .Func("replace", &RegularExpression::replace)
        .Func("matched", &RegularExpression::matched)
        .Func("matchesCount", &RegularExpression::matchesCount)
        .Func("getSubStrings", &RegularExpression::getSubStrings)
        .Func("split", &RegularExpression::split)
        .Func("splitWithLimitOffset", &RegularExpression::splitWithLimitOffset)
        .Func("splitWithLimitStartEndOffset", &RegularExpression::splitWithLimitStartEndOffset)
        .Func("getMatchStart", &RegularExpression::getMatchStart)
        .Func("getMatchEnd", &RegularExpression::getMatchEnd)
        .Func("getEntireMatchStart", &RegularExpression::getEntireMatchStart)
        .Func("getEntireMatchEnd", &RegularExpression::getEntireMatchEnd)
    );

    root.Func("CRegExp", CreateRegExp);
}

}
