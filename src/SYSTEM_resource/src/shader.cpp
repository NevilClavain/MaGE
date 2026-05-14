
/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2026
*
* This file is part of MaGE.
*
*    MaGE is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    MaGE is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with MaGE.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/* -*-LIC_END-*- */

#include <md5.h>
#include "shader.h"

using namespace mage;

Shader::Shader(int p_type) :
m_type(p_type)
{
}

Shader::Shader(const Shader& p_other)
{
    m_source_id = p_other.m_source_id;
    m_resource_uid = p_other.m_resource_uid;
    
    //m_content = p_other.m_content;
    //m_contentSize = p_other.m_contentSize;

    m_file_content = p_other.m_file_content;
    m_file_content_size = p_other.m_file_content_size;

    m_code = p_other.m_code;
    m_code_size = p_other.m_code_size;

    m_type = p_other.m_type;

    m_state_mutex.lock();
    p_other.m_state_mutex.lock();
    m_state = p_other.m_state;
    p_other.m_state_mutex.unlock();
    m_state_mutex.unlock();

    m_generic_arguments = p_other.m_generic_arguments;
    m_vectorarray_arguments = p_other.m_vectorarray_arguments;
}

std::string Shader::getResourceUID() const
{
    return m_resource_uid;
}

std::string Shader::getSourceID() const
{
    return m_source_id;
}

void Shader::setSourceID(const std::string& p_source_id)
{
    m_source_id = p_source_id;
    compute_resource_uid();
}

const char* Shader::getFileContent() const
{
    return m_file_content;
}

size_t Shader::getFileContentSize() const
{
    return m_file_content_size;
}

void Shader::setFileContent(const char* p_file_content, size_t p_file_content_size)
{
    m_file_content = p_file_content;
    m_file_content_size = p_file_content_size;

    compute_content_hash();
}

char* Shader::getCode() const
{
    return m_code;
}

size_t Shader::getCodeSize() const
{
    return m_code_size;
}

void Shader::setCode(char* p_code, size_t p_code_size)
{
    m_code = p_code;
    m_code_size = p_code_size;
}

std::string Shader::getContentHash() const
{
    return m_content_hash;
}

Shader::State Shader::getState() const
{
    m_state_mutex.lock();
    const auto state{ m_state };
    m_state_mutex.unlock();
    return state;
}

void Shader::setState(State p_state)
{
    m_state_mutex.lock();
    m_state = p_state;
    m_state_mutex.unlock();
}

int Shader::getType() const
{
    return m_type;
}

void Shader::addGenericArgument(const GenericArgument& p_arg)
{
    m_generic_arguments.push_back(p_arg);
}

std::vector<Shader::GenericArgument> Shader::getGenericArguments() const
{
    return m_generic_arguments;
}

void Shader::addVectorArrayArgument(const VectorArrayArgument& p_arg)
{
    m_vectorarray_arguments.push_back(p_arg);
}

std::vector<Shader::VectorArrayArgument>& Shader::vectorArrayArgumentsAccess()
{
    return m_vectorarray_arguments;
}

const std::vector<Shader::VectorArrayArgument>& Shader::getVectorArrayArguments() const
{
    return m_vectorarray_arguments;
}

void Shader::compute_resource_uid()
{
    m_resource_uid = std::string("CONTENT_FROM_FILE_") + m_source_id;
}

void Shader::compute_content_hash()
{
    MD5 md5;
    const std::string hash{ md5.digestMemory((BYTE*)m_file_content, m_file_content_size) };

    m_content_hash = hash;
}