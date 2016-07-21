#ifndef ARG3_DB_BIND_MAPPING_H
#define ARG3_DB_BIND_MAPPING_H

#include "bindable.h"

#ifndef ENHANCED_PARAMETER_MAPPING
#include "exception.h"
#endif

namespace arg3
{
    namespace db
    {
#ifdef ENHANCED_PARAMETER_MAPPING
        /*!
         * a binding that supports mapping named parameters to indexed parameters
         */
        class bind_mapping : public bindable
        {
           protected:
            typedef std::unordered_map<std::string, std::set<size_t>> type;

           public:
            bind_mapping();
            bind_mapping(const bind_mapping &other);
            bind_mapping(bind_mapping &&other);
            virtual ~bind_mapping();
            bind_mapping &operator=(const bind_mapping &other);
            bind_mapping &operator=(bind_mapping &&other);

            /*!
             * prepares the bindings for a sql string.  should call prepare_params()
             * @param sql the sql string with parameters
             * @param the number of indexed parameters in the sql already
             * @return the reformatted sql
             * @throws binding_error if the sql contains mixed named and indexed parameters
             */
            std::string prepare(const std::string &sql, size_t max_index);

            bind_mapping &bind(const std::string &name, const sql_value &value);

            bool is_named() const;

            /*!
             * reset all the binding
             */
            virtual void reset();

           protected:
            void add_named_param(const std::string &name, size_t index);
            void rem_named_param(const std::string &name, size_t index);
            std::set<size_t> get_named_param_indexes(const std::string &name);

            type mappings_;
        };
#else
        class bind_mapping : public bindable
        {
           public:
            using bindable::bindable;

            bind_mapping &bind(const std::string &name, const sql_value &value)
            {
                throw database_exception("not implemented");
            }

            virtual void reset()
            {
            }
        };
#endif
    }
}


#endif