

/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2025
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

#pragma once

#include <string>
#include <mutex>
#include <vector>

#include "buffer.h"

#include "matrix.h"
#include "tvector.h"



namespace mage
{
    //fwd decl
    class ResourceSystem;
    class D3D11System;

    static constexpr int            vertexShader{ 0 };
    static constexpr int            pixelShader{ 1 };

    class Shader
    {
    public:

        struct GenericArgument
        {
            std::string                 argument_id;
            
            std::string                 argument_type;
            core::maths::Matrix         matrix;
            core::maths::Real4Vector    real4vector;

            int                         shader_register{ -1 };
        };


        struct VectorArrayArgument
        {
            int                                     start_shader_register{ -1 };
            std::vector<core::maths::Real4Vector>   array;
        };

        Shader() = delete;
        Shader(int p_type);
        Shader(const Shader& p_other);

        Shader& operator=(const Shader& p_other)
        {
            m_source_id = p_other.m_source_id;
            m_resource_uid = p_other.m_resource_uid;
            m_content = p_other.m_content;
            m_contentSize = p_other.m_contentSize;
            m_code = p_other.m_code;
            m_type = p_other.m_type;

            m_state_mutex.lock();
            p_other.m_state_mutex.lock();
            m_state = p_other.m_state;
            p_other.m_state_mutex.unlock();
            m_state_mutex.unlock();

            m_generic_arguments = p_other.m_generic_arguments;
            m_vectorarray_arguments = p_other.m_vectorarray_arguments;

            return *this;
        }

        enum class State
        {
            INIT,
            BLOBLOADING,
            BLOBLOADED,
            RENDERERLOADING,
            RENDERERLOADED,
        };

        std::string getContent() const;
        void setContent(const std::string& p_content);

        std::string getResourceUID() const;

        std::string getSourceID() const;

        size_t getContentSize() const;
        void setContentSize(size_t p_contentSize);
        State getState() const;
        
        int getType() const;

        const core::Buffer<char>& getCode() const;
        
        void addGenericArgument(const GenericArgument& p_arg);
        std::vector<GenericArgument> getGenericArguments() const;

        void addVectorArrayArgument(const VectorArrayArgument& p_arg);

        const std::vector<VectorArrayArgument>& getVectorArrayArguments() const;
        std::vector<VectorArrayArgument>& vectorArrayArgumentsAccess();

    private:
        
        std::string                         m_resource_uid;       // shader content source unique identifier
        std::string                         m_source_id;
        std::string                         m_content;

        size_t                              m_contentSize{ 0 };

        int                                 m_type; //0 = vertex shader, 1 = pixel shader

        core::Buffer<char>                  m_code;

        mutable std::mutex	                m_state_mutex;
        State                               m_state{ State::INIT };

        std::vector<GenericArgument>        m_generic_arguments;
        std::vector<VectorArrayArgument>    m_vectorarray_arguments;

        // IF NEW MEMBERS HERE :
        // UPDATE COPY CTOR AND OPERATOR !!!!!!



        void setState(State p_state);
        void setCode(const core::Buffer<char>& p_code);

        void compute_resource_uid();

        friend class mage::ResourceSystem;
        friend class mage::D3D11System;
    };
}

