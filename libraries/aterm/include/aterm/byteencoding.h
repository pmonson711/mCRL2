#ifndef BYTEENCODING_H_
#define BYTEENCODING_H_

#include "atypes.h"

namespace aterm
{

size_t BEserializeMultiByteInt(int i, char* c);

int BEdeserializeMultiByteInt(char* c, size_t* i);

} // namespace aterm

#endif /*BYTEENCODING_H_*/
