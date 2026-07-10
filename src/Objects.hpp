#pragma once

#include <string>
#include <mutex>
#include <string_view>
#include <iostream>
#include <thread>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <chrono>
#include <future>
#include <optional>
#include <type_traits>
#include <regex>
#include <stdexcept>
#include <exception>
#include <utility>
#include <execution>
#include <algorithm>

constexpr size_t USER_NAME_SIZE = 4;
constexpr size_t DISPLAY_NAME_SIZE = 3;
constexpr size_t USER_NAME_MAX_SIZE = 30;
constexpr size_t DISPLAY_NAME_MAX_SIZE = 20;

constexpr uint8_t INFANT = 13;
constexpr uint8_t MINOR = 17;
constexpr uint8_t TEEN = 20;
constexpr uint8_t ADULT = 24;

constexpr size_t FOLLOWER_LIST_SIZE = 30000u;

using namespace std::chrono;

inline uint64_t GLOBAL_COUNTER { 0ull };
inline std::vector<uint64_t> DELETED_IDS_HOLDER;
inline std::mutex DELETED_USER_IDS_MUTEX;


namespace UTL
{
    inline void h_errorCalls ( const char* message, bool call )
    {
        if ( call )
        {
            std::cerr << message << std::endl;
        } else
        {
            std::cout << message << std::endl;
        }
    }

    static std::array<std::string, 20> FILTERED_KEYWORDS {
        "fuck", "shit", "fag", "kill", "rape", "bitch",
        "bitsh", "kys", "gypsy", "wog", "cracker", "slut",
        "slag", "whore", "hoe", "sandmonkey", "hentai",
        "porn", "bob", "slave"
    };

    inline bool h_filterString ( std::string requestedString )
    {
        return std::any_of(
            FILTERED_KEYWORDS.begin(),
            FILTERED_KEYWORDS.end(),
            [requestedString] ( const std::string& indexedKeyword ) -> auto
            {
                if ( requestedString.find(indexedKeyword) != std::string::npos ) return true;
                if ( FILTERED_KEYWORDS.back() == indexedKeyword ) return false;
            }
        );
    }

    inline bool h_validateEmail ( std::string requestedString )
    {
        std::regex pattern(R"(^[\w-\.]+@([\w-]+\.)+[\w-]{2,6}$)");
        return std::regex_match(requestedString, pattern);
    }

    template < typename __TYPE_CONTAINER >
    struct DebugIterator final
    {
        __TYPE_CONTAINER::iterator m_currentIteration;

        DebugIterator(
            __TYPE_CONTAINER::iterator beginOfIteration
        ) : m_currentIteration(beginOfIteration)
        {}
    };
}

#ifndef OBJECTS_HPP
#define OBJECTS_HPP
namespace OBL
{
    enum class Roles
    {
        NONE = 0,
        DEFAULT = 1,
        CUSTOMER_SUPPORT = 20,
        MODERATOR = 30,
        ADMINISTRATOR = 50,
        OWNER = 100
    };

    enum class ErrorCodes
    {
        INAPPROPRIATE,
        RULES,
        NAMEBOUNDARIES,
        DISPLAYNAMEBOUNDARIES,
        SUCCESS,
        ALREADY,
        FALSE,
        TRUE,
        ERROR
    };

    enum class AgeIdentifier
    {
        INFANT,
        MINOR,
        TEEN,
        ADULT,
        ERROR
    };

    struct UserInfo final
    {
        std::string m_firstName;
        std::string m_secondName;
        std::string m_lastName;
        std::string m_displayName;
        std::string m_personalEmail;

        Roles m_userRole { 
            Roles::DEFAULT
        };

        uint8_t m_age { 0u };
        const uint64_t m_userID {
            []() -> uint64_t
            {
                std::lock_guard<std::mutex> thread_lock(DELETED_USER_IDS_MUTEX);

                if ( !DELETED_IDS_HOLDER.empty() )
                {
                    uint64_t currentID = DELETED_IDS_HOLDER.front();
                    DELETED_IDS_HOLDER.erase(DELETED_IDS_HOLDER.begin());
                    return currentID;
                }

                return ++GLOBAL_COUNTER;
            }()
        };

        seconds m_diplsayTimeStamp {
            duration_cast<seconds>(system_clock::now().time_since_epoch())
        };
        const system_clock::time_point m_createdDate {
            system_clock::now()
        };
        std::optional<system_clock::time_point> m_lastLogin;
    };

    class User final
    {
        private:
        UserInfo* m_userInfo { new UserInfo };
        mutable std::mutex m_userFriendsMutex;
        mutable std::mutex m_userFriendsMutex2;
        mutable std::mutex m_userFollowersMutex;
        mutable std::mutex m_userFollowersMutex2;
        mutable std::mutex m_userFriendRequestMutex;
        mutable std::mutex m_userFriendRequestMutex2;

        ErrorCodes debugCallBackDisplayName ( std::string_view requestedChange ) noexcept(true)
        {
            const size_t len = requestedChange.length();
            if ( len <= DISPLAY_NAME_SIZE || len > DISPLAY_NAME_MAX_SIZE ) return ErrorCodes::DISPLAYNAMEBOUNDARIES;

            if ( UTL::h_filterString(std::string(requestedChange)) ) return ErrorCodes::INAPPROPRIATE;

            return ErrorCodes::SUCCESS;
        }

        ErrorCodes debugCallBackPersonalInformation (
            std::string_view requestedChangeForFirstName,
            std::string_view requestedChangeForSecondName,
            std::string_view requestedChangeForLastName
        ) noexcept(true)
        {
            const size_t firstName_Len = requestedChangeForFirstName.length();
            const size_t lastName_Len = requestedChangeForLastName.length();
            if ( firstName_Len <= USER_NAME_SIZE || firstName_Len > USER_NAME_MAX_SIZE ||
                    lastName_Len <= USER_NAME_SIZE || lastName_Len > USER_NAME_MAX_SIZE
            )   return ErrorCodes::NAMEBOUNDARIES;

            if ( UTL::h_filterString(std::string(requestedChangeForFirstName)) || UTL::h_filterString(std::string(requestedChangeForSecondName)) ||
                    UTL::h_filterString(std::string(requestedChangeForLastName))
            )   return ErrorCodes::INAPPROPRIATE;

            return ErrorCodes::SUCCESS;
        }

        explicit User(
            std::string FIRST_NAME,
            std::string SECOND_NAME,
            std::string LAST_NAME,
            std::string DISPLAY_NAME,
            std::string PERSONAL_EMAIL,
            Roles USER_ROLE = Roles::DEFAULT
        ) noexcept(true)
        {
            m_userInfo->m_firstName = FIRST_NAME;
            m_userInfo->m_secondName = SECOND_NAME;
            m_userInfo->m_lastName = LAST_NAME;
            m_userInfo->m_displayName = DISPLAY_NAME;
            m_userInfo->m_personalEmail = PERSONAL_EMAIL;
            m_userInfo->m_userRole = USER_ROLE;

            m_followerList.resize(FOLLOWER_LIST_SIZE);
        }

        protected:
        mutable std::unordered_map<uint64_t, std::shared_ptr<User>> m_friendList;
        mutable std::unordered_map<uint64_t, std::shared_ptr<User>> m_friendRequestsList;
        mutable std::vector<std::shared_ptr<User>> m_followerList;

        std::optional<User> getFriend ( uint64_t requestedUser ) const
        {
            std::lock_guard<std::mutex> thread_lock(m_userFriendsMutex);
            try
            {
                return *m_friendList.at(requestedUser);
            } catch ( const std::out_of_range& e )
            {
                return std::nullopt;
            }
        }

        std::optional<User> getFollower ( const User& requestedFollower ) noexcept(true)
        {
            std::lock_guard<std::mutex> thread_lock(m_userFollowersMutex2);

            for ( UTL::DebugIterator<std::vector<std::shared_ptr<User>>> It(m_followerList.begin()); It.m_currentIteration != m_followerList.end(); It.m_currentIteration++)
            {
                if ( (**(It.m_currentIteration)).m_userInfo->m_userID == requestedFollower.m_userInfo->m_userID )
                {
                    return **(It.m_currentIteration);
                }
            }

            return std::nullopt;
        }

        ErrorCodes insertFriend ( uint64_t userID, const User& requestedFriend ) const
        {
            std::lock_guard<std::mutex> thread_lock(m_userFriendsMutex2);
            try
            {
                m_friendList.at(userID);
                return ErrorCodes::ALREADY;
            } catch ( const std::out_of_range& e )
            {
                std::shared_ptr<User> refFriend = std::make_shared<User>(requestedFriend);
                m_friendList[userID] = refFriend;
                return ErrorCodes::SUCCESS;
            }
        }

        ErrorCodes insertFollower ( const User& requestedFollower ) noexcept(true)
        {
            std::lock_guard<std::mutex> thread_lock(m_userFollowersMutex);

            for ( decltype(auto) It = m_followerList.begin(); It != m_followerList.end(); It++ )
            {
                if ( (**It).m_userInfo->m_userID == requestedFollower.m_userInfo->m_userID )
                {
                    return ErrorCodes::ALREADY;
                }
            }

            std::shared_ptr<User> followerRef = std::make_shared<User>(requestedFollower);
            m_followerList.push_back(followerRef);
            return ErrorCodes::SUCCESS;
        }

        ErrorCodes canSendFriendRequest ( const User& requestedUser ) const
        {
            std::lock_guard<std::mutex> thread_lock(m_userFriendRequestMutex);

            try
            {
                if ( m_friendList.at(requestedUser.m_userInfo->m_userID) )
                    return ErrorCodes::FALSE;
                    
            } catch ( const std::out_of_range& e )
            {
                return ErrorCodes::TRUE;
            }
            return ErrorCodes::ERROR;
        }

        void sendFriendRequest ( const User& requestedUser )
        {
            std::lock_guard<std::mutex> thread_lock(m_userFriendRequestMutex2);

            ErrorCodes code = canSendFriendRequest(requestedUser);

            if ( code == ErrorCodes::TRUE )
            {
                
            }
        }

        public:
        static std::optional<User> create (
            std::string FIRST_NAME,
            std::string SECOND_NAME,
            std::string LAST_NAME,
            std::string DISPLAY_NAME,
            std::string PERSONAL_EMAIL,
            Roles USER_ROLE = Roles::DEFAULT
        ) noexcept(true)
        {

            if ( FIRST_NAME.length() <= USER_NAME_SIZE || LAST_NAME.length() <= USER_NAME_SIZE || DISPLAY_NAME.length() <= DISPLAY_NAME_SIZE ||
                    UTL::h_filterString(FIRST_NAME) || UTL::h_filterString(SECOND_NAME) || UTL::h_filterString(LAST_NAME) || UTL::h_filterString(DISPLAY_NAME) )
                return std::nullopt;

            return User(std::move(FIRST_NAME), 
                std::move(SECOND_NAME), 
                std::move(LAST_NAME),
                std::move(DISPLAY_NAME),
                std::move(PERSONAL_EMAIL),
                std::move(USER_ROLE)
            );
        }

        static ErrorCodes debugCreateCallBack ( 
            std::string_view FIRST_NAME,
            std::string_view SECOND_NAME,
            std::string_view LAST_NAME,
            std::string_view DISPLAY_NAME,
            std::string_view PERSONAL_EMAIL,
            Roles USER_ROLE = Roles::DEFAULT
        ) noexcept(true)
        {
            if ( FIRST_NAME.length() <= USER_NAME_SIZE || LAST_NAME.length() <= USER_NAME_SIZE )
                return ErrorCodes::NAMEBOUNDARIES;

            if ( DISPLAY_NAME.length() <= DISPLAY_NAME_SIZE )
                return ErrorCodes::DISPLAYNAMEBOUNDARIES;

            if ( UTL::h_filterString(std::string(FIRST_NAME)) || UTL::h_filterString(std::string(SECOND_NAME)) || 
                UTL::h_filterString(std::string(LAST_NAME)) || UTL::h_filterString(std::string(DISPLAY_NAME)) )
                return ErrorCodes::INAPPROPRIATE;

            
            return ErrorCodes::SUCCESS;
        }

        User(
            User&& otherUser
        ) noexcept(true) 
        {
            if (&otherUser != this)
            {
                m_userInfo->m_firstName = otherUser.m_userInfo->m_firstName;
                m_userInfo->m_secondName = otherUser.m_userInfo->m_secondName;
                m_userInfo->m_lastName = otherUser.m_userInfo->m_lastName;
                m_userInfo->m_displayName = otherUser.m_userInfo->m_displayName;
                m_userInfo->m_personalEmail = otherUser.m_userInfo->m_personalEmail;
                m_userInfo->m_userRole = otherUser.m_userInfo->m_userRole;

                otherUser.m_userInfo->m_firstName = nullptr;
                otherUser.m_userInfo->m_secondName = nullptr;
                otherUser.m_userInfo->m_lastName = nullptr;
                otherUser.m_userInfo->m_displayName = nullptr;
                otherUser.m_userInfo->m_personalEmail = nullptr;
                otherUser.m_userInfo->m_userRole = Roles::NONE;
            }
        }

        User(
            const User& otherUser
        ) noexcept(true)
        {
            if (&otherUser != this)
            {
                m_userInfo->m_firstName = otherUser.m_userInfo->m_firstName;
                m_userInfo->m_secondName = otherUser.m_userInfo->m_secondName;
                m_userInfo->m_lastName = otherUser.m_userInfo->m_lastName;
                m_userInfo->m_displayName = otherUser.m_userInfo->m_displayName;
                m_userInfo->m_personalEmail = otherUser.m_userInfo->m_personalEmail;
                m_userInfo->m_userRole = otherUser.m_userInfo->m_userRole;
            }
        }

        User& operator= ( User&& otherUser ) noexcept(true)
        {
            if (&otherUser != this)
            {
                m_userInfo->m_firstName = otherUser.m_userInfo->m_firstName;
                m_userInfo->m_secondName = otherUser.m_userInfo->m_secondName;
                m_userInfo->m_lastName = otherUser.m_userInfo->m_lastName;
                m_userInfo->m_displayName = otherUser.m_userInfo->m_displayName;
                m_userInfo->m_personalEmail = otherUser.m_userInfo->m_personalEmail;
                m_userInfo->m_userRole = otherUser.m_userInfo->m_userRole;

                otherUser.m_userInfo->m_firstName = nullptr;
                otherUser.m_userInfo->m_secondName = nullptr;
                otherUser.m_userInfo->m_lastName = nullptr;
                otherUser.m_userInfo->m_displayName = nullptr;
                otherUser.m_userInfo->m_personalEmail = nullptr;
                otherUser.m_userInfo->m_userRole = Roles::NONE;
            }

            return *this;
        }

        User& operator= ( const User& otherUser ) noexcept(true)
        {
            if (&otherUser != this)
            {
                m_userInfo->m_firstName = otherUser.m_userInfo->m_firstName;
                m_userInfo->m_secondName = otherUser.m_userInfo->m_secondName;
                m_userInfo->m_lastName = otherUser.m_userInfo->m_lastName;
                m_userInfo->m_displayName = otherUser.m_userInfo->m_displayName;
                m_userInfo->m_personalEmail = otherUser.m_userInfo->m_personalEmail;
                m_userInfo->m_userRole = otherUser.m_userInfo->m_userRole;
            }

            return *this;
        }

        ~User()
        {
            DELETED_IDS_HOLDER.push_back(m_userInfo->m_userID);
            delete m_userInfo;
        }

        ErrorCodes setDisplayName ( std::string requestedChange ) noexcept(true)
        {
            ErrorCodes code = debugCallBackDisplayName(requestedChange);

            if ( code == ErrorCodes::SUCCESS )
                m_userInfo->m_displayName = requestedChange;

            return code;
        }

        ErrorCodes setPersonalInformation ( 
            std::string requestedChangeForFirstName,
            std::string requestedChangeForSecondName,
            std::string requestedChangeForLastName
        ) noexcept(true)
        {
            ErrorCodes code = debugCallBackPersonalInformation(requestedChangeForFirstName,
                requestedChangeForSecondName,
                requestedChangeForLastName);
            
            if ( code == ErrorCodes::SUCCESS )
            {
                m_userInfo->m_firstName = requestedChangeForFirstName;
                m_userInfo->m_secondName = requestedChangeForSecondName;
                m_userInfo->m_lastName = requestedChangeForLastName;
            }

            return code;
        }

        AgeIdentifier findAgeGroup () noexcept(true)
        {
            const uint8_t age = m_userInfo->m_age;
            if ( age <= INFANT ) 
            {
                return AgeIdentifier::INFANT;
            } else if ( age <= MINOR )
            {
                return AgeIdentifier::MINOR;
            } else if ( age <= TEEN )
            {
                return AgeIdentifier::TEEN;
            } else if ( age <= ADULT )
            {
                return AgeIdentifier::ADULT;
            } else {
                return AgeIdentifier::ERROR;
            }
        }

        std::array < std::string , 3 > showPersonalInformation () noexcept(true)
        {
            return std::array { m_userInfo->m_firstName, m_userInfo->m_secondName, m_userInfo->m_lastName };
        }

        void recordLastLogin () noexcept(true)
        {
            m_userInfo->m_lastLogin = system_clock::now();
        }


    };

    class UserDB final
    {};

    
}
#endif
