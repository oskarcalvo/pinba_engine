/* Copyright (c) 2007-2009 Antony Dovgal <tony@daylessday.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/* $Id: ha_pinba.h,v 1.31.6.5 2009/03/23 15:21:20 tony Exp $ */

#ifdef USE_PRAGMA_INTERFACE
#pragma interface			/* gcc class implementation */
#endif

#define PINBA_MAX_KEYS 2

enum {
	PINBA_TABLE_UNKNOWN,
	PINBA_TABLE_REQUEST,
	PINBA_TABLE_TIMER,
	PINBA_TABLE_TIMERTAG,
	PINBA_TABLE_TAG,
	PINBA_TABLE_INFO,
	PINBA_TABLE_REPORT1, /* group by script_name */
	PINBA_TABLE_REPORT2, /* group by virtual host */
	PINBA_TABLE_REPORT3, /* group by hostname */
	PINBA_TABLE_REPORT4, /* group by virtual host, script_name */
	PINBA_TABLE_REPORT5, /* group by hostname, script_name */
	PINBA_TABLE_REPORT6, /* group by hostname, virtual_host */
	PINBA_TABLE_REPORT7, /* group by hostname, virtual_host and script_name */
	PINBA_TABLE_TAG_INFO, /* tag report grouped by custom tag */
	PINBA_TABLE_TAG2_INFO, /* tag report grouped by 2 custom tags */
	PINBA_TABLE_TAG_REPORT, /* tag report grouped by script_name and custom tag */
	PINBA_TABLE_TAG2_REPORT /* tag report grouped by script_name and 2 custom tags */
};

typedef struct pinba_index_st { /* {{{ */
	union {
		size_t ival;
		struct {
			unsigned char *val;
			uint len;
		} str;
	};
	size_t position;
} pinba_index_st;
/* }}} */

/* this thing is SHAREd between threads! */
typedef struct pinba_share_st { /* {{{ */
	char *table_name;
	uint table_name_length;
	uint use_count;
	THR_LOCK lock;
	unsigned char table_type;
	char **params;
	int params_num;
} PINBA_SHARE;
/* }}} */

/*
   Class definition for the storage engine.
   New instance of the class is created for each new thread.
   */
class ha_pinba: public handler
{
	THR_LOCK_DATA lock;    /* MySQL lock */
	PINBA_SHARE *share;    /* Shared lock info */
	char key_built_buffer[2048];
	size_t current_key_length;
	char *current_key;
	unsigned char *rec_buff;
	size_t alloced_rec_buff_length;
	size_t rec_buff_length;
	pinba_index_st this_index[PINBA_MAX_KEYS];

	int read_row_by_key(unsigned char *buf, uint active_index, const unsigned char *key, uint key_len, int exact);
	int read_next_row(unsigned char *buf, uint active_index);
	int read_index_first(unsigned char *buf, uint active_index);

	inline int requests_fetch_row(unsigned char *buf, size_t index, size_t *new_index);
	inline int timers_fetch_row(unsigned char *buf, size_t index, size_t *new_index, int exact);
	inline int timers_fetch_row_by_request_id(unsigned char*, size_t index, size_t*new_index);

	inline int tags_fetch_row(unsigned char *buf, size_t index, size_t *new_index);
	inline int tags_fetch_row_by_name(unsigned char*, const unsigned char *name, uint name_len);

	inline int tag_values_fetch_next(unsigned char *buf, size_t *index, size_t *position);
	inline int tag_values_fetch_by_timer_id(unsigned char *buf); 

	inline int info_fetch_row(unsigned char *buf);
	inline int report1_fetch_row(unsigned char *buf);
	inline int report2_fetch_row(unsigned char *buf);
	inline int report3_fetch_row(unsigned char *buf);
	inline int report4_fetch_row(unsigned char *buf);
	inline int report5_fetch_row(unsigned char *buf);
	inline int report6_fetch_row(unsigned char *buf);
	inline int report7_fetch_row(unsigned char *buf);
	inline int tag_info_fetch_row(unsigned char *buf);
	inline int tag2_info_fetch_row(unsigned char *buf);
	inline int tag_report_fetch_row(unsigned char *buf);
	inline int tag2_report_fetch_row(unsigned char *buf);

	public:
	ha_pinba(handlerton *hton, TABLE_SHARE *table_arg);
	~ha_pinba()
	{
	}

	const char *table_type() const { 
		return "PINBA"; 
	}
	
	const char **bas_ext() const;
	ulonglong table_flags() const
	{
		return (HA_NO_AUTO_INCREMENT|HA_BINLOG_ROW_CAPABLE|HA_NO_TRANSACTIONS|HA_STATS_RECORDS_IS_EXACT);
	}

	ulong index_flags(uint inx, uint part, bool all_parts) const
	{
		return HA_READ_NEXT | HA_READ_PREV | HA_KEYREAD_ONLY;
	}


	const char *index_type(uint inx)
	{
		return ("HASH");
	}

	uint max_supported_record_length() const { return HA_MAX_REC_LENGTH; }
	uint max_supported_keys()          const { return PINBA_MAX_KEYS; }
	uint max_supported_key_parts()     const { return 1; }
	uint max_supported_key_length()    const { return 256; }
	
	/* methods */

	int open(const char *name, int mode, uint test_if_locked);    //required
	int close(void);                                              //required
	int rnd_init(bool scan);                                      //required
	int rnd_next(unsigned char *buf);                             //required
	int rnd_pos(unsigned char * buf, unsigned char *pos);         //required
	int rnd_end();
	int index_init(uint keynr, bool sorted);
	int index_read(unsigned char * buf, const unsigned char * key,	uint key_len, enum ha_rkey_function find_flag);
	int index_next(unsigned char * buf);
	int index_prev(unsigned char * buf);
	/* 
	 Even though the docs say index_first() is not required, 'SELECT * FROM <table> WHERE <index> > N' 
	 is not going to work without it. See mysql_ha_read() in sql_handler.cc, line ~548.
	 */
	int index_first(unsigned char * buf); 

	void position(const unsigned char *record);                   //required
	int info(uint);                                               //required

	int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info); //required
	int delete_all_rows();

	THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to, enum thr_lock_type lock_type);     //required
};

/* 
 * vim600: sw=4 ts=4 fdm=marker
 */