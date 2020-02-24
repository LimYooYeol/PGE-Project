/*
 * Platformer Game Engine by Wohlstand, a free platform for game making
 * Copyright (c) 2014-2020 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QPixmap>
#include <QImage>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <memory>

#include <common_features/logger.h>
#include <FileMapper/file_mapper.h>

#include "graphics_funcs.h"

#include <FreeImageLite.h>
#include <QtDebug>


FIBITMAP *GraphicsHelps::loadImage(QString file, bool convertTo32bit)
{
#if  defined(__unix__) || defined(__APPLE__) || defined(_WIN32)
    FileMapper fileMap;

    if(!fileMap.open_file(file.toUtf8().data()))
        return nullptr;

    FIMEMORY *imgMEM = FreeImage_OpenMemory(reinterpret_cast<unsigned char *>(fileMap.data()),
                                            static_cast<unsigned int>(fileMap.size()));
    FREE_IMAGE_FORMAT formato = FreeImage_GetFileTypeFromMemory(imgMEM);

    if(formato  == FIF_UNKNOWN)
        return nullptr;

    FIBITMAP *img = FreeImage_LoadFromMemory(formato, imgMEM, 0);
    FreeImage_CloseMemory(imgMEM);
    fileMap.close_file();

    if(!img)
        return nullptr;

#else
    FREE_IMAGE_FORMAT formato = FreeImage_GetFileType(file.toUtf8().data(), 0);

    if(formato  == FIF_UNKNOWN)
        return nullptr;

    FIBITMAP *img = FreeImage_Load(formato, file.toUtf8().data());

    if(!img)
        return nullptr;

#endif

    if(convertTo32bit)
    {
        FIBITMAP *temp;
        temp = FreeImage_ConvertTo32Bits(img);

        if(!temp)
            return nullptr;

        FreeImage_Unload(img);
        img = temp;
    }

    return img;
}

void GraphicsHelps::mergeWithMask(FIBITMAP *image, QString pathToMask, QPixmap *maskFallback)
{
    if(!image) return;

    if(!QFile::exists(pathToMask) && !maskFallback)
        return; //Nothing to do.

    FIBITMAP *mask = loadImage(pathToMask, true);
    if(!mask && maskFallback)
        getMaskFromRGBA(*maskFallback, mask);

    if(!mask) return;//Nothing to do.

    unsigned int img_w = FreeImage_GetWidth(image);
    unsigned int img_h = FreeImage_GetHeight(image);
    unsigned int mask_w = FreeImage_GetWidth(mask);
    unsigned int mask_h = FreeImage_GetHeight(mask);
    BYTE *img_bits  = FreeImage_GetBits(image);
    BYTE *mask_bits = FreeImage_GetBits(mask);
    BYTE *FPixP = img_bits;
    BYTE *SPixP = mask_bits;
    RGBQUAD Npix = {0x00, 0x00, 0x00, 0xFF};   //Destination pixel color
    unsigned short newAlpha = 255; //Calculated destination alpha-value

    unsigned int ym = mask_h - 1;
    unsigned int y = img_h - 1;
    while(1)
    {
        FPixP = img_bits + (img_w * y * 4);
        SPixP = mask_bits + (mask_w * ym * 4);
        for(unsigned int x = 0; (x < img_w) && (x < mask_w); x++)
        {
            Npix.rgbBlue = ((SPixP[FI_RGBA_BLUE] & 0x7F) | FPixP[FI_RGBA_BLUE]);
            Npix.rgbGreen = ((SPixP[FI_RGBA_GREEN] & 0x7F) | FPixP[FI_RGBA_GREEN]);
            Npix.rgbRed = ((SPixP[FI_RGBA_RED] & 0x7F) | FPixP[FI_RGBA_RED]);
            newAlpha = 255 - ((static_cast<unsigned short>(SPixP[FI_RGBA_RED]) +
                               static_cast<unsigned short>(SPixP[FI_RGBA_GREEN]) +
                               static_cast<unsigned short>(SPixP[FI_RGBA_BLUE])) / 3);

            if((SPixP[FI_RGBA_RED] > 240u) //is almost White
               && (SPixP[FI_RGBA_GREEN] > 240u)
               && (SPixP[FI_RGBA_BLUE] > 240u))
                newAlpha = 0;

            newAlpha += ((short(FPixP[FI_RGBA_RED]) +
                          short(FPixP[FI_RGBA_GREEN]) +
                          short(FPixP[FI_RGBA_BLUE])) / 3);

            if(newAlpha > 255)
                newAlpha = 255;

            FPixP[FI_RGBA_BLUE]  = Npix.rgbBlue;
            FPixP[FI_RGBA_GREEN] = Npix.rgbGreen;
            FPixP[FI_RGBA_RED]   = Npix.rgbRed;
            FPixP[FI_RGBA_ALPHA] = static_cast<BYTE>(newAlpha);
            FPixP += 4;
            SPixP += 4;
        }

        if(y == 0 || ym == 0)
            break;
        y--; ym--;
    }

    FreeImage_Unload(mask);
}






QPixmap GraphicsHelps::mergeToRGBA(QPixmap image, QImage mask)
{
    if(mask.isNull())
        return image;

    if(image.isNull())
        return image;

    QImage target = image.toImage();

    if(target.format() != QImage::Format_ARGB32)
        target = target.convertToFormat(QImage::Format_ARGB32);

    mergeToRGBA_BitWise(target, mask);
    return QPixmap::fromImage(target);
}

void GraphicsHelps::mergeToRGBA(QPixmap &img, QImage &mask, QString path, QString maskpath)
{
    if(path.isNull())
        return;

    QImage target;
    target = loadQImage(path);

    if(target.format() != QImage::Format_ARGB32)
        target = target.convertToFormat(QImage::Format_ARGB32);

    if(maskpath.isNull())
    {
        img.convertFromImage(target);
        return;
    }

    mask = loadQImage(maskpath);
    mergeToRGBA_BitWise(target, mask);
    img.convertFromImage(target);
}

void GraphicsHelps::getMaskFromRGBA(const QPixmap &srcimage, QImage &mask)
{
    unsigned int img_w   = static_cast<unsigned int>(srcimage.width());
    unsigned int img_h   = static_cast<unsigned int>(srcimage.height());

    QImage image = srcimage.toImage();
    mask = QImage(int(img_w), int(img_h), QImage::Format_ARGB32);
    QRgb Fpix;
    for(unsigned int y = 0; (y < img_h); y++)
    {
        for(unsigned int x = 0; (x < img_w); x++)
        {
            Fpix = image.pixel(int(x), int(y));
            int gray = (255 - qAlpha(Fpix));
            mask.setPixel(int(x), int(y), qRgba(gray, gray, gray, 0xFF));
        }
    }
}

void GraphicsHelps::getMaskFromRGBA(const QPixmap &srcimage, FIBITMAP *&mask)
{
    unsigned int img_w   = static_cast<unsigned int>(srcimage.width());
    unsigned int img_h   = static_cast<unsigned int>(srcimage.height());

    QImage image = srcimage.toImage();

    mask = FreeImage_AllocateT(FIT_BITMAP,
                               int(img_w), int(img_h),
                               32,
                               FI_RGBA_RED_MASK,
                               FI_RGBA_GREEN_MASK,
                               FI_RGBA_BLUE_MASK);

    RGBQUAD Npix = {0x0, 0x0, 0x0, 0xFF};

    for(unsigned int y = 0; (y < img_h); y++)
    {
        for(unsigned int x = 0; (x < img_w); x++)
        {
            uint8_t gray = uint8_t(255 - qAlpha(image.pixel(int(x), (int(img_h) - 1) - int(y)) ));
            Npix.rgbRed  = gray;
            Npix.rgbGreen = gray;
            Npix.rgbBlue = gray;
            Npix.rgbReserved = 0xFF;
            FreeImage_SetPixelColor(mask,  x, y, &Npix);
        }
    }
}

//Implementation of Bitwise merging of bit-mask to RGBA image
void GraphicsHelps::mergeToRGBA_BitWise(QImage &image, QImage mask)
{
    if(mask.isNull())
        return;

    if(image.isNull())
        return;

    //bool isWhiteMask = true;
    //QImage target(image.width(), image.height(), QImage::Format_ARGB32);

    //QImage newmask = mask.convertToFormat(QImage::Format_ARGB32);
    //newmask = newmask.convertToFormat(QImage::Format_ARGB32);
    //    if(target.size()!= mask.size())
    //    {
    //        mask = mask.copy(0, 0, target.width(), target.height());
    //    }

    for(int y = 0; (y < image.height()) && (y < mask.height()); y++)
    {
        for(int x = 0; (x < image.width()) && (x < mask.width()); x++)
        {
            QColor Fpix = QColor(image.pixel(x, y));
            QColor Dpix = QColor(qRgb(128, 128, 128));
            QColor Spix = QColor(mask.pixel(x, y));
            QColor Npix;
            Npix.setAlpha(255);
            //AND
            Npix.setRed(Dpix.red() & Spix.red());
            Npix.setGreen(Dpix.green() & Spix.green());
            Npix.setBlue(Dpix.blue() & Spix.blue());
            //OR
            Npix.setRed(Fpix.red() | Npix.red());
            Npix.setGreen(Fpix.green() | Npix.green());
            Npix.setBlue(Fpix.blue() | Npix.blue());
            //            isWhiteMask &= ( (Spix.red()>240) //is almost White
            //                             &&(Spix.green()>240)
            //                             &&(Spix.blue()>240));
            //Calculate alpha-channel level
            int newAlpha = 255 - ((Spix.red() + Spix.green() + Spix.blue()) / 3);

            if((Spix.red() > 240) //is almost White
               && (Spix.green() > 240)
               && (Spix.blue() > 240))
                newAlpha = 0;

            newAlpha = newAlpha + ((Fpix.red() + Fpix.green() + Fpix.blue()) / 3);

            if(newAlpha > 255)
                newAlpha = 255;

            Npix.setAlpha(newAlpha);
            image.setPixel(x, y, Npix.rgba());
        }
    }
}

void GraphicsHelps::loadMaskedImage(QString rootDir, QString in_imgName, QString &out_maskName, QPixmap &out_Img, QImage &, QString &out_errStr)
{
    loadMaskedImage(rootDir, in_imgName, out_maskName, out_Img, out_errStr);
}

void GraphicsHelps::loadMaskedImage(QString rootDir, QString in_imgName, QString &out_maskName, QPixmap &out_Img, QString &out_errStr)
{
    if(in_imgName.isEmpty())
    {
        out_errStr = "Image filename isn't defined";
        return;
    }

    if(!QFile::exists(rootDir + in_imgName))
    {
        out_errStr = "image file is not exist: " + rootDir + in_imgName;
        return;
    }

    out_maskName = in_imgName;
    int i = out_maskName.size() - 1;

    for(; i > 0; i--)
    {
        if(out_maskName[i] == '.')
        {
            out_maskName.insert(i, 'm');
            break;
        }
    }

    QImage target;

    if(i == 0)
    {
        out_maskName = "";
        loadQImage(target, rootDir + in_imgName);
    }
    else
        loadQImage(target, rootDir + in_imgName, rootDir + out_maskName);

    if(target.isNull())
    {
        out_errStr = "Broken image file " + rootDir + in_imgName;
        return;
    }

    //GraphicsHelps::mergeToRGBA(out_Img, out_Mask, rootDir+in_imgName, rootDir + out_maskName);
    out_Img = QPixmap::fromImage(target);

    if(out_Img.isNull())
    {
        out_errStr = "Broken image file " + rootDir + in_imgName;
        return;
    }

    out_errStr = "";
}

QPixmap GraphicsHelps::loadPixmap(QString file)
{
    return QPixmap::fromImage(loadQImage(file));
}

QImage GraphicsHelps::loadQImage(QString file)
{
    if(file.startsWith(':'))
    {
        //Load from resources!
        QImage image = QImage(file);
        return image;
    }
    else    //Load via FreeImage
    {
        FIBITMAP *img = loadImage(file, true);

        if(img)
        {
            BYTE *bits = FreeImage_GetBits(img);
            int width  = int(FreeImage_GetWidth(img));
            int height = int(FreeImage_GetHeight(img));
            QImage target(width, height, QImage::Format_ARGB32);

            for(int y = height - 1; y >= 0; y--)
            {
                for(int x = 0; x < width; x++)
                {
                    target.setPixel(x, y, qRgba(bits[FI_RGBA_RED],
                                                bits[FI_RGBA_GREEN],
                                                bits[FI_RGBA_BLUE],
                                                bits[FI_RGBA_ALPHA]));
                    bits += 4;
                }
            }

            FreeImage_Unload(img);
            return target;
        }
        else
            return QImage();
    }
}

void GraphicsHelps::loadQImage(QImage &target, QString file, QString maskPath, QPixmap *fallbackMask)
{
    if(file.startsWith(':'))
    {
        //Load from resources!
        target = QImage(file);
        return;
    }
    else    //Load via FreeImage
    {
        FIBITMAP *img = loadImage(file, true);

        if(img)
        {
            mergeWithMask(img, maskPath, fallbackMask);
            BYTE *bits = FreeImage_GetBits(img);
            int width  = int(FreeImage_GetWidth(img));
            int height = int(FreeImage_GetHeight(img));
            target = QImage(width, height, QImage::Format_ARGB32);

            for(int y = height - 1; y >= 0; y--)
            {
                for(int x = 0; x < width; x++)
                {
                    target.setPixel(x, y, qRgba(bits[FI_RGBA_RED],
                                                bits[FI_RGBA_GREEN],
                                                bits[FI_RGBA_BLUE],
                                                bits[FI_RGBA_ALPHA]));
                    bits += 4;
                }
            }

            FreeImage_Unload(img);
            return;
        }
        else
            return;
    }
}

void GraphicsHelps::squareImageR(QPixmap &imageInOut, QSize targetSize)
{
    if(imageInOut.isNull())
        return;

    if(targetSize.isNull())
    {
        if(imageInOut.width() > imageInOut.height())
        {
            targetSize.setWidth(imageInOut.height());
            targetSize.setHeight(imageInOut.height());
        }
        else
        {
            targetSize.setWidth(imageInOut.width());
            targetSize.setHeight(imageInOut.width());
        }
    }

    QPixmap source;

    if((targetSize.width() < imageInOut.width()) || (targetSize.height() < imageInOut.height()))
        source = imageInOut.scaled(targetSize, Qt::KeepAspectRatio, Qt::FastTransformation);
    else
        source = imageInOut;

    imageInOut = QPixmap(targetSize);
    imageInOut.fill(Qt::transparent);
    QPainter p(&imageInOut);
    int targetX = qRound(((qreal(imageInOut.width()) - qreal(source.width())) / 2));
    int targetY = qRound(((qreal(imageInOut.height()) - qreal(source.height())) / 2));
    p.drawPixmap(targetX, targetY, source.width(), source.height(), source);
    p.end();
}

QPixmap GraphicsHelps::drawDegitFont(int number)
{
    QPixmap font = QPixmap(":/fonts/degits.png");
    QString text = QString::number(number);
    QPixmap img(text.size() * 18, 16);
    img.fill(Qt::transparent);
    QPainter p(&img);

    for(int i = 0; i < text.size(); i++)
        p.drawPixmap(QRect(18 * i, 0, 16, 16), font, QRect(0, QString(text[i]).toInt() * 16, 16, 16));

    p.end();
    return img;
}
