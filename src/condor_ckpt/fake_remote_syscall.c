/***************************************************************
 *
 * Copyright (C) 1990-2007, Condor Team, Computer Sciences Department,
 * University of Wisconsin-Madison, WI.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License.  You may
 * obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************/


 

/*
  This routine is for linking with the standalone checkpointing library.  Since
  programs linked that way should never attempt remote system calls, it is
  a fatal error if this routine ever gets called.
*/

#include "condor_common.h"
#include "condor_debug.h"
#include "syscall_numbers.h"

int REMOTE_syscall( int syscall_num, ... );

int
REMOTE_syscall( int syscall_num, ... )
{
	EXCEPT( "Linked for standalone checkpointing, but called REMOTE_syscall(%d)", syscall_num);
	return -1;
}
