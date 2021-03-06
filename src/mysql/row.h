/*!
 * Mysql specific implementations of a row
 * @file row.h
 */
#ifndef CODA_DB_MYSQL_ROW_H
#define CODA_DB_MYSQL_ROW_H

#include <mysql/mysql.h>
#include <vector>
#include "../row.h"

namespace coda::db::mysql {
  class session;
  class binding;

  /*!
   *  a mysql specific implementation of a row
   */
  class row : public row_impl {
   private:
    MYSQL_ROW row_;
    std::shared_ptr<MYSQL_RES> res_;
    std::shared_ptr<mysql::session> sess_;
    size_t size_;

   public:
    /*!
     * @param db the database in use
     * @param res the query result
     * @param row the row values
     */
    row(const std::shared_ptr<mysql::session> &db, const std::shared_ptr<MYSQL_RES> &res, MYSQL_ROW row);

    /* non-copyable boilerplate */
    ~row() override = default;
    row(const row &other) = delete;
    row(row &&other) noexcept = default;
    row &operator=(const row &other) = delete;
    row &operator=(row &&other) noexcept = default;

    /* row_impl overrides */
    std::string column_name(size_t position) const override;
    column_type column(size_t position) const override;
    column_type column(const std::string &name) const override;
    size_t size() const noexcept override;
    bool is_valid() const noexcept override;
  };

  /*!
   *  a mysql specific implementation of a row using prepared statements
   */
  class stmt_row : public row_impl {
   private:
    std::shared_ptr<mysql::binding> fields_;
    std::shared_ptr<MYSQL_RES> metadata_;
    std::shared_ptr<MYSQL_STMT> stmt_;
    std::shared_ptr<mysql::session> sess_;
    size_t size_;

   public:
    /*!
     * @param db the database in use
     * @param stmt the query statement
     * @param metadata the query meta data
     * @param fields the bindings for the statement
     */
    stmt_row(const std::shared_ptr<mysql::session> &sess, const std::shared_ptr<MYSQL_STMT> &stmt,
             const std::shared_ptr<MYSQL_RES> &metadata, const std::shared_ptr<mysql::binding> &fields);

    /* non-copyable boilerplate */
    ~stmt_row() override = default;
    stmt_row(const stmt_row &other) = delete;
    stmt_row(stmt_row &&other) noexcept;
    stmt_row &operator=(const stmt_row &other) = delete;
    stmt_row &operator=(stmt_row &&other) noexcept;

    /* row_impl overrides */
    std::string column_name(size_t position) const override;
    column_type column(size_t position) const override;
    column_type column(const std::string &name) const override;
    size_t size() const noexcept override;
    bool is_valid() const noexcept override;
  };
}  // namespace coda::db::mysql

#endif
