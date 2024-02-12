#pragma once
#include<pthread.h>
#include<queue>

namespace core{
    template<typename T>
    class SafeQueue{
        private:
        pthread_mutex_t m_mtx;
        std::queue<T> m_queData;
        public:
        SafeQueue():m_mtx(),m_queData(){
            pthread_mutex_init(&m_mtx, NULL);
        }
        void Clear(){
            pthread_mutex_lock(&m_mtx);
            while(!m_queData.empty())
                m_queData.pop();
            pthread_mutex_unlock(&m_mtx);
        }
        bool IsEmpty(){
            pthread_mutex_lock(&m_mtx);
            bool isempty = m_queData.empty();
            pthread_mutex_unlock(&m_mtx);
            return isempty;
        }
        size_t Count(){
            pthread_mutex_lock(&m_mtx);
            size_t count = m_queData.size();
            pthread_mutex_unlock(&m_mtx);
            return count;
        }
        void Push(const T& data){
            pthread_mutex_lock(&m_mtx);
            m_queData.push(data);
            pthread_mutex_unlock(&m_mtx);
        }
        void Pop(T& outItem){
            pthread_mutex_lock(&m_mtx);
            if(m_queData.empty())
                return;
            outItem = m_queData.front();
            m_queData.pop();
            pthread_mutex_unlock(&m_mtx);
        }
        void Seek(T& outItem){
            pthread_mutex_lock(&m_mtx);
            if(m_queData.empty())
                return;
            outItem = m_queData.front();
            pthread_mutex_unlock(&m_mtx);
        }
        ~SafeQueue(){
            pthread_mutex_destroy(&m_mtx);
        }
    };
}