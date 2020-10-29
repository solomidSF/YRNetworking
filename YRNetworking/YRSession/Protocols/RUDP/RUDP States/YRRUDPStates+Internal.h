//
// YRRUDPStates+Internal.h
//
// The MIT License (MIT)
//
// Copyright (c) 2020 Yurii Romanchenko
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __YRRUDPStates_Internal__
#define __YRRUDPStates_Internal__	

#define STATE_DEFAULT_PREFIX yr_session_state

#define STATE_FP_PREFIX(name) STATE_COMBINE(STATE_COMBINE(STATE_DEFAULT_PREFIX, STATE_PREFIX), name)
#define STATE_COMBINE(prefix, name) STATE_COMBINE2(prefix, name)
#define STATE_COMBINE2(prefix, name) prefix ## _ ## name

#define STATE_FP_IMPL(name, ...) \
	YR_FP_IMPL(STATE_FP_PREFIX(name), __VA_ARGS__)

#define STATE_DECL(represented_state) \
	(YRRUDPState) { \
			represented_state, \
			STATE_FP_PREFIX(onEnter), \
			STATE_FP_PREFIX(onExit), \
			{STATE_FP_PREFIX(invalidate), STATE_FP_PREFIX(destroy)}, \
			{STATE_FP_PREFIX(connect), STATE_FP_PREFIX(wait), STATE_FP_PREFIX(close), STATE_FP_PREFIX(send), STATE_FP_PREFIX(receive)}, \
			{STATE_FP_PREFIX(syn), STATE_FP_PREFIX(rst), STATE_FP_PREFIX(nul), STATE_FP_PREFIX(eack), STATE_FP_PREFIX(regular), STATE_FP_PREFIX(invalid)} \
		}

#endif // __YRRUDPStates_Internal__
