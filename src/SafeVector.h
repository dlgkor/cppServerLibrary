#pragma once
#include<pthread.h>
#include<vector>

namespace core{
    template<typename T>
    class SafeVector{
        private:
        pthread_mutex_t m_mtx;
        std::vector<T> m_vecData;
        public:
        SafeVector():m_mtx(),m_vecData(){
            pthread_mutex_init(&m_mtx, NULL);
        }
        void Clear(){
            pthread_mutex_lock(&m_mtx);
            while(!m_vecData.empty())
                m_vecData.pop_back();
            pthread_mutex_unlock(&m_mtx);
        }
        bool IsEmpty(){
            pthread_mutex_lock(&m_mtx);
            bool isempty = m_vecData.empty();
            pthread_mutex_unlock(&m_mtx);
            return isempty;
        }
        size_t Count(){
            pthread_mutex_lock(&m_mtx);
            size_t count = m_vecData.size();
            pthread_mutex_unlock(&m_mtx);
            return count;
        }
        void PushBack(const T& data){
            pthread_mutex_lock(&m_mtx);
            m_vecData.push_back(data);
            pthread_mutex_unlock(&m_mtx);
        }
        void At(T& outItem, int num){
            pthread_mutex_lock(&m_mtx);
            if(m_vecData.empty())
                return;
            outItem = m_vecData.at(num);
            pthread_mutex_unlock(&m_mtx);
        }
        void Erase(int num){
            pthread_mutex_lock(&m_mtx);
            if(m_vecData.empty())
                return;
            m_vecData.erase(m_vecData.begin() + num);
            pthread_mutex_unlock(&m_mtx);
        }
        ~SafeVector(){
            pthread_mutex_destroy(&m_mtx);
        }
    };
}