#include <QvSFImage.h>

// Can't use macro, since we define a constructor.

QvSFImage::QvSFImage()
{
    size[0] = size[1] = 0;
    numComponents = 0;
    bytes = NULL;
}

QvSFImage::~QvSFImage()
{
    if (bytes != NULL)
	delete [] bytes;
}
QvBool
QvSFImage::readValue(QvInput *in)
{
    if (! in->read(size[0]) ||
	! in->read(size[1]) ||
	! in->read(numComponents))
	return FALSE;
    
    if (bytes != NULL)
	delete [] bytes;
    bytes = new unsigned char[size[0] * size[1] * numComponents];

    int byte = 0;
    for (int i = 0; i < size[0] * size[1]; i++) {
	unsigned long l;
	if (! in->read(l))
	    return FALSE;
	for (int j = 0; j < numComponents; j++)
	    bytes[byte++] =
		(unsigned char) ((l >> (8*(numComponents-j-1))) & 0xFF);
    }
    return TRUE;
}

void QvSFImage::propagate(QvField *dest)
{
	*((QvSFImage*)dest) = *this;
}

QvSFImage& QvSFImage::operator=( const QvSFImage& v )
{
	if(&v != this)
	{
		delete [] bytes;
		size[0] = v.size[0];
		size[1] = v.size[1];
		numComponents = v.numComponents;
		int nBytes = size[0] * size[1] * numComponents;
    	bytes = new unsigned char[nBytes];
		memcpy(bytes, v.bytes, nBytes);
	}
	return *this;
}
