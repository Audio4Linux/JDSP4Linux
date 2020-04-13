#ifndef PHANTOMSTYLE_H
#define PHANTOMSTYLE_H
#include <QtWidgets/qcommonstyle.h>

class PhantomStylePrivate;
class PhantomStyle : public QCommonStyle {
  Q_OBJECT
public:
  PhantomStyle();
  ~PhantomStyle();

  enum PhantomPrimitiveElement {
    Phantom_PE_IndicatorTabNew = PE_CustomBase + 1,
    Phantom_PE_ScrollBarSliderVertical,
    Phantom_PE_WindowFrameColor,
  };

  QPalette standardPalette() const override;
  void drawPrimitive(PrimitiveElement elem, const QStyleOption* option,
                     QPainter* painter,
                     const QWidget* widget = nullptr) const override;
  void drawControl(ControlElement ce, const QStyleOption* option,
                   QPainter* painter, const QWidget* widget) const override;
  int pixelMetric(PixelMetric metric, const QStyleOption* option = nullptr,
                  const QWidget* widget = nullptr) const override;
  void drawComplexControl(ComplexControl control,
                          const QStyleOptionComplex* option, QPainter* painter,
                          const QWidget* widget) const override;
  QRect subElementRect(SubElement r, const QStyleOption* opt,
                       const QWidget* widget = nullptr) const override;
  QSize sizeFromContents(ContentsType type, const QStyleOption* option,
                         const QSize& size,
                         const QWidget* widget) const override;
  SubControl hitTestComplexControl(ComplexControl cc,
                                   const QStyleOptionComplex* opt,
                                   const QPoint& pt,
                                   const QWidget* w = nullptr) const override;
  QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt,
                       SubControl sc, const QWidget* widget) const override;
  QPixmap generatedIconPixmap(QIcon::Mode iconMode, const QPixmap& pixmap,
                              const QStyleOption* opt) const override;
  int styleHint(StyleHint hint, const QStyleOption* option = nullptr,
                const QWidget* widget = nullptr,
                QStyleHintReturn* returnData = nullptr) const override;
  QRect itemPixmapRect(const QRect& r, int flags,
                       const QPixmap& pixmap) const override;
  void drawItemPixmap(QPainter* painter, const QRect& rect, int alignment,
                      const QPixmap& pixmap) const override;
  void
  drawItemText(QPainter* painter, const QRect& rect, int flags,
               const QPalette& pal, bool enabled, const QString& text,
               QPalette::ColorRole textRole = QPalette::NoRole) const override;
  void polish(QWidget* widget) override;
  void polish(QApplication* app) override;
  void polish(QPalette& pal) override;
  void unpolish(QWidget* widget) override;
  void unpolish(QApplication* app) override;

protected:
  PhantomStylePrivate* d;
};
#endif
